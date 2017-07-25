#include "reservations.h"
#include "safety.h"
#include "track.h"
#include "track_node.h"
#include "ns.h"
#include "clock.h"

namespace {
constexpr auto ReverseReservation = 200;
constexpr auto SwitchClearance = 200;
}

void savecur();
void restorecur();

Reservations::Reservations(const SafetyState &safety, TrainServer &ts)
    : safety_(safety)
    , trainServer_(ts) {
}

void Reservations::flipSwitchesInReservation(Train t, const TrainReservation &r) {
    for (Size i = 0; i < r.length; i++) {
        const auto &node = Track().nodes[r.reservations[i]];
        if (node.type == NODE_BRANCH) {
            const auto &gasp = safety_.trains.get(t).gasp.gradient[node.num];
            if (gasp == SwitchState::Straight || gasp == SwitchState::Curved) {
                cmdSetSwitch(node.num, gasp);
            }
        }
    }
}

bool Reservations::checkForRerverseInReservation(Train t,
        const TrainReservation &r, Distance *out) {
    Distance d = 0; // TODO: does not take into account offset into the node
    for (Size i = 0; i < r.length; i++) {
        d += safety_.nodeEdge(r.reservations[i]).dist;
        const auto &node = Track().nodes[r.reservations[i]];
        const auto &n = node.type;
        if (n == NODE_BRANCH || n == NODE_MERGE) {
            const auto &gasp = safety_.trains.get(t).gasp.gradient[node.num];
            if ((n == NODE_BRANCH && gasp == SwitchState::Neither) ||
                    (n == NODE_MERGE && (gasp == SwitchState::Straight ||
                                         gasp == SwitchState::Curved))) {
                *out = d + SwitchClearance;
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
    const auto &startNode = Track().nodes[trModel.lastSensor.value()];
    bwprintf(COM2, "%d %s\r\n", trModel.lastSensor.value(), startNode.name);

    // Reserve forwards, to next sensor + stopping distance
    int forwardDistance = 0;
    bool foundNextSensor = false;
    auto forward = &startNode;
    bool reservedAll = true;
    while (!(foundNextSensor && forwardDistance > trModel.stoppingDistance)) {
        if (forward->type == NODE_EXIT) {
            reservedAll = false;
            break;
        }
        auto dir = DIR_AHEAD;
        if (forward->type == NODE_BRANCH) {
            if (safety_.switches[forward->num] == SwitchState::Curved) {
                dir = DIR_CURVED;
            }
        }
        if (!reserveNode(train, forward - Track().nodes)) {
            reservedAll = false;
            break;
        }
        if (foundNextSensor) {
            forwardDistance += forward->edge[dir].dist;
        }
        if (forward->edge[dir].dest->type == NODE_SENSOR) {
            foundNextSensor = true;
        }
        forward = forward->edge[dir].dest;
    }

    // Flip switches, but only on the forwards reservation.
    flipSwitchesInReservation(train, r);
    // Reverse trains, but only on the forwards reservation.
    Distance d;
    if (!r.isReversing && checkForRerverseInReservation(train, r, &d)) {
        // Ignore the case where there is not sufficient stopping distance for
        // a reverse. It should not happen and the routing will find another
        // route.
        if (d > trModel.stoppingDistance && trModel.velocity > 0) {
            Time duration = (d - trModel.stoppingDistance) * VELOCITY_CONSTANT /
                    trModel.velocity + 1;
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
