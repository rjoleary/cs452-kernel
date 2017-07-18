#include "reservations.h"
#include "model.h"
#include "track.h"
#include "track_node.h"
#include "ns.h"
#include "clock.h"

namespace {
constexpr auto ReverseReservation = 200;
}

void savecur();
void restorecur();

Reservations::Reservations(const ModelState &mod)
    : model(mod) {
}

void Reservations::printReservations() const {
    int top = 3;
    int left = 49;
    for (const auto &tr : trainReservations.keys()) {
        savecur();
        bwprintf(COM2, "\033[%d;%dH\033[KTrain %d: ", top, left, tr);
        const auto &reses = trainReservations.get(tr);
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

bool Reservations::reserve(Train train, NodeIdx node) {
    for (auto &reses : trainReservations.values()) {
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
    auto &myRes = trainReservations.get(train);
    myRes.reservations[myRes.length++] = node;
    return true;
}

bool Reservations::sensorTriggered(Train train, Sensor sensor) {
    trainReservations.get(train).length = 0;

    const auto &startNode = Track().nodes[sensor.value()];
    bwprintf(COM2, "%d %s\r\n", sensor.value(), startNode.name);

    // Reserve forwards, to next sensor + stopping distance
    int forwardDistance = 0;
    bool foundNextSensor = false;
    auto forward = &startNode;
    while (!(foundNextSensor && forwardDistance >= model.trains.get(train).stoppingDistance)) {
        if (forward->type == NODE_EXIT) return false;
        auto dir = DIR_AHEAD;
        if (forward->type == NODE_BRANCH) {
            if (model.switches[forward->num] == 'C')
                dir = DIR_CURVED;
        }
        if (!reserve(train, forward - Track().nodes)) return false;
        if (foundNextSensor) {
            forwardDistance += forward->edge[dir].dist;
        }
        if (forward->edge[dir].dest->type == NODE_SENSOR) {
            foundNextSensor = true;
        }
        forward = forward->edge[dir].dest;
    }

    // Reserve backwards
    int backwardsDistance = 0;
    auto reverse = startNode.reverse;
    while (backwardsDistance < ReverseReservation && reverse->type != NODE_EXIT) {
        auto dir = DIR_AHEAD;
        // If it was a branch, get the correct previous node
        if (reverse->type == NODE_BRANCH) {
            if (model.switches[reverse->num] == 'C')
                dir = DIR_CURVED;
        }
        backwardsDistance += reverse->edge[dir].dist;
        reverse = reverse->edge[dir].dest;
        if (!reserve(train, reverse->reverse - Track().nodes)) return false;
    }

    return true;
}

void Reservations::doReservations(Train train, Sensor sensor, Speed speed) {
    TrainServer ts;
    Waitlist newList;
    if (!sensorTriggered(train, sensor)) {
        newList.items[newList.length++] = {train, sensor, speed};
        ts.cmdSetSpeed(train, 0);
        bwprintf(COM2, "Train %d waitlisted\r\n", train);
    }
    for (Size i = 0; i < waitlist.length; ++i) {
        if (!sensorTriggered(waitlist.items[i].train, waitlist.items[i].sensor)) {
            newList.items[newList.length++] = waitlist.items[i];
            bwprintf(COM2, "Train %d rewaitlisted\r\n", waitlist.items[i].train);
        }
        else {
            ts.cmdSetSpeed(waitlist.items[i].train, waitlist.items[i].speed);
            bwprintf(COM2, "Train %d unwaitlisted %d\r\n",
                    waitlist.items[i].train, waitlist.items[i].speed);
        }
    }
    waitlist = newList;
    // see if we can move anything in waitlist
}

bool Reservations::hasReservation(Train train, NodeIdx idx) const {
    if (!trainReservations.has(train)) return false;
    const TrainReservation &tr = trainReservations.get(train);
    for (Size i = 0; i < tr.length; i++) {
        // TODO: time-based hasReservation
        if (tr.reservations[i] == idx) {
            return true;
        }
    }
    return false;
}
