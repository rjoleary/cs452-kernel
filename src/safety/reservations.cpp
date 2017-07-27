#include "reservations.h"
#include "safety.h"
#include "track.h"
#include "track_node.h"
#include "ns.h"
#include "clock.h"

namespace {
constexpr auto ReverseReservation = 200;
constexpr auto SwitchClearance = 100;
}

void savecur();
void restorecur();

Reservations::Reservations(const SafetyState &safety)
    : safety_(safety)
{}

NodeIdx Reservations::clearReversing(Train tr) {
    trainReservations_.get(tr).isReversing = false;
    INFOF(60, "Train %d reversed and node set to %d\n", tr, trainReservations_.get(tr).reverseNode);
    return trainReservations_.get(tr).reverseNode;
}

void Reservations::flipSwitchesInReservation(Train t, const TrainReservation &r) {
    for (Size i = 0; i < r.length; i++) {
        const auto &node = Track().nodes[r.reservations[i]];
        if (node.type == NODE_BRANCH || node.type == NODE_MERGE) {
            const auto &gasp = safety_.trains.get(t).gasp.gradient[node.num];
            if (gasp == SwitchState::Straight || gasp == SwitchState::Curved) {
                cmdSetSwitch(node.num, gasp);
            }
        }
    }
}

bool Reservations::checkForStopInReservation(Train t,
        TrainReservation &r, Distance *out) {
    Distance d = 0; // TODO: does not take into account offset into the node
    const auto &gasp = safety_.trains.get(t).gasp;
    for (Size i = 0; i < r.length; i++) {
        if (d > r.totalDistance - SwitchClearance) break;
        d += safety_.nodeEdge(r.reservations[i], t).dist;
        if (gasp.end.nodeIdx == r.reservations[i]) {
            d += gasp.end.offset;
            *out = d;
            return true;
        }
    }
    return false;
}

bool Reservations::checkForReverseInReservation(Train t,
        TrainReservation &r, Distance *out) {
    Distance d = 0; // TODO: does not take into account offset into the node
    for (Size i = 0; i < r.length; i++) {
        if (d > r.totalDistance - SwitchClearance) break;
        d += safety_.nodeEdge(r.reservations[i], t).dist;
        const auto &node = Track().nodes[r.reservations[i]];
        const auto &n = node.type;
        if (n == NODE_BRANCH || n == NODE_MERGE) {
            const auto &gasp = safety_.trains.get(t).gasp.gradient[node.num];
            if ((n == NODE_BRANCH && gasp == SwitchState::Neither) ||
                    (n == NODE_MERGE && (gasp == SwitchState::Straight ||
                                         gasp == SwitchState::Curved))) {
                *out = d + SwitchClearance;
                r.reverseNode = Track().nodes[r.reservations[i]].reverse - Track().nodes;
                return true;
            }
        }
    }
    return false;
}

void Reservations::printReservations() const {
    int top = 3;
    int left = 49;
    for (const auto &tr : trainReservations_.keys()) {
        savecur();
        const auto &trModel = safety_.trains.get(tr);
        bwprintf(COM2, "\033[%d;%dH\033[KTrain %d, stop %d: pos: %s +%d",
                top, left,
                tr,
                trModel.stoppingDistance,
                Track().nodes[trModel.position.nodeIdx].name,
                trModel.position.offset);
        const auto &reses = trainReservations_.get(tr);
        for (Size i = 0; i < reses.length; i++) {
            const auto &res = reses.reservations[i];
            const auto &node = Track().nodes[res];
            bwputstr(COM2, node.name);
            bwputstr(COM2, "->");
        }
        restorecur();
        flush(COM2);
        top++;
    }
}

bool Reservations::reserveNode(Train train, NodeIdx node) {
    for (auto &reses : trainReservations_.values()) {
        for (Size j = 0; j < reses.length; ++j) {
            const auto &res = reses.reservations[j];
            if (res == node
                    || Track().nodes[res].edge[DIR_AHEAD].dest->reverse - Track().nodes == node
                    || (Track().nodes[res].type == NODE_BRANCH
                        ? Track().nodes[res].edge[DIR_CURVED].dest->reverse - Track().nodes == node
                        : false)) {
                return false;
            }
        }
    }
    auto &myRes = trainReservations_.get(train);
    myRes.reservations[myRes.length++] = node;
    return true;
}

bool Reservations::reserveForTrain(Train train) {
    auto &r = trainReservations_.get(train);
    r.length = 0;

    const auto &trModel = safety_.trains.get(train);
    const auto &startNode = Track().nodes[trModel.lastKnownNode];

    // Reserve forwards, to next sensor + stopping distance
    Distance forwardDistance = 0, totalDistance = 0;
    bool foundNextSensor = false;
    auto forward = &startNode;
    bool reservedAll = true;
    bool exitNode = false;
    while (!(foundNextSensor && forwardDistance > trModel.stoppingDistance + SwitchClearance)) {
        if (forward->type == NODE_EXIT) {
            r.reverseNode = forward->reverse - Track().nodes;
            exitNode = true;
            break;
        }
        const auto &next = safety_.nodeEdge(forward - Track().nodes, train);

        if (!reserveNode(train, forward - Track().nodes)) {
            reservedAll = false;
            break;
        }
        totalDistance += next.dist;
        if (foundNextSensor) {
            forwardDistance += next.dist;
        }
        if (next.dest->type == NODE_SENSOR) {
            foundNextSensor = true;
        }
        forward = next.dest;
    }
    r.totalDistance = totalDistance;

    // Flip switches, but only on the forwards reservation.
    flipSwitchesInReservation(train, r);

    // Stop trains, but only on the forwards reservation.
    Distance d;
    if (!r.isReversing && !r.isStopping && checkForStopInReservation(train, r, &d)) {
        // Ignore the case where there is not sufficient stopping distance. It
        // should not happen and the routing will find another route.
        if (d > trModel.stoppingDistance && trModel.velocity > 0) {
            Time duration = (d - trModel.stoppingDistance) * VELOCITY_CONSTANT /
                    trModel.velocity + 1;
            // TODO: these two train operations are not atomic
            trainServer_.addDelay(train, duration);
            trainServer_.setTrainSpeed(train, 0);
            r.isStopping = true;
        }
    }

    // Reverse trains, but only on the forwards reservation.
    auto reverseInRes = checkForReverseInReservation(train, r, &d);
    if (!r.isStopping && !r.isReversing && (reverseInRes || exitNode)) {
        // Ignore the case where there is not sufficient stopping distance for
        // a reverse. It should not happen and the routing will find another
        // route.
        if (exitNode && !reverseInRes) d = totalDistance;
        if (d > trModel.stoppingDistance && trModel.velocity > 0) {
            Time duration = (d - trModel.stoppingDistance) * VELOCITY_CONSTANT /
                    trModel.velocity + 1;
            INFOF(58, "DISTANCE: %d\r\n", d);
            INFOF(59, "DURATION: %d\r\n", duration);
            // TODO: these two train operations are not atomic
            trainServer_.addDelay(train, duration);
            trainServer_.reverseTrain(train);
            r.isReversing = true;
        }
    }

    // Reserve backwards
    int backwardsDistance = 0;
    auto reverse = startNode.reverse;
    while (backwardsDistance < ReverseReservation && reverse->type != NODE_EXIT) {
        auto dir = DIR_AHEAD;
        // If it was a branch, get the correct previous node
        if (reverse->type == NODE_BRANCH) {
            if (safety_.switches[reverse->num] == SwitchState::Curved) {
                dir = DIR_CURVED;
            }
        }
        backwardsDistance += reverse->edge[dir].dist;
        reverse = reverse->edge[dir].dest;
        if (!reserveNode(train, reverse->reverse - Track().nodes)) {
            reservedAll = false;
            break;
        }
    }

    return reservedAll;
}

void Reservations::processUpdate(Train train) {
    TrainServer ts;
    Waitlist newList;
    const auto &trModel = safety_.trains.get(train);
    if (!reserveForTrain(train)) {
        newList.trains[newList.length++] = train;
        ts.setTrainSpeed(train, 0);
        bwprintf(COM2, "Train %d waitlisted\r\n", train);
    }
    for (Size i = 0; i < waitlist.length; ++i) {
        if (!reserveForTrain(waitlist.trains[i])) {
            newList.trains[newList.length++] = waitlist.trains[i];
            bwprintf(COM2, "Train %d rewaitlisted\r\n", waitlist.trains[i]);
        }
        else {
            ts.setTrainSpeed(waitlist.trains[i], trModel.speed);
            bwprintf(COM2, "Train %d unwaitlisted\r\n",
                    waitlist.trains[i]);
        }
    }
    waitlist = newList;
    // see if we can move anything in waitlist
}

bool Reservations::hasReservation(Train train, NodeIdx idx) const {
    if (!trainReservations_.has(train)) {
        return false;
    }
    const TrainReservation &tr = trainReservations_.get(train);
    for (Size i = 0; i < tr.length; i++) {
        // TODO: time-based hasReservation
        if (tr.reservations[i] == idx) {
            return true;
        }
    }
    return false;
}
