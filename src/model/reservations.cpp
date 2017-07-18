#include "reservations.h"
#include "model.h"
#include "track.h"
#include "track_node.h"
#include "ns.h"
#include "clock.h"

namespace {
constexpr auto ReverseReservation = 200;
constexpr auto TimeError = 1;
}

Reservations::Reservations(const ModelState &mod)
    : model(mod) {
}

void Reservations::printReservations() const {
    int top = 3;
    int left = 38;
    for (const auto &tr : trainReservations.keys()) {
        bwprintf(COM2, "\033[%d;%dH\033[K;Train %d: ", top, left, tr);
        const auto &reses = trainReservations.get(tr);
        for (Size i = 0; i < reses.length; i++) {
            const auto &res = reses.reservations[i];
            const auto &node = Track.nodes[res.node];
            bwputstr(COM2, node.name);
            bwputstr(COM2, "->");
        }
        flush(COM2);
        top++;
    }
}

bool Reservations::reserve(Train train, NodeIdx node, int dist, int length) {
    // TODO: Fix time
    auto clockServ = ctl::whoIs(ctl::names::ClockServer).asValue();
    auto velocity = model.trains.get(train).velocity;
    auto startTime = ctl::time(clockServ).asValue() + dist/velocity,
         endTime = startTime + length/velocity;
    startTime -= TimeError;
    endTime += TimeError;
    for (auto &reses : trainReservations.values()) {
        for (Size j = 0; j < reses.length; ++j) {
            const auto &res = reses.reservations[j];
            if (res.node == node
                    || Track.nodes[res.node].edge[DIR_AHEAD].dest->reverse - Track.nodes == node
                    || (Track.nodes[res.node].type == NODE_BRANCH
                        ? Track.nodes[res.node].edge[DIR_CURVED].dest->reverse - Track.nodes == node
                        : false)) {
                if (res.end < startTime) continue;
                if (res.start > endTime) continue;
                return false;
            }
        }
    }
    auto &myRes = trainReservations.get(train);
    myRes.reservations[myRes.length++] = {node, startTime, endTime};
    return true;
}

bool Reservations::sensorTriggered(Train train, Sensor sensor) {
    trainReservations.get(train).length = 0;

    const auto &startNode = Track.nodes[sensor.value()];

    // Reserve backwards
    int backwardsDistance = 0;
    auto reverse = startNode.reverse;
    while (backwardsDistance < ReverseReservation) {
        auto dir = DIR_AHEAD;
        // If it was a branch, get the correct previous node
        if (reverse->type == NODE_BRANCH) {
            if (model.switches[reverse->num] == 'C')
                dir = DIR_CURVED;
        }
        auto dist = reverse->edge[dir].dist;
        backwardsDistance += reverse->edge[dir].dist;
        reverse = reverse->edge[dir].dest;
        if (!reserve(train, reverse->reverse - Track.nodes, -backwardsDistance, dist)) return false;
    }

    // Reserve forwards, to next sensor + stopping distance
    int forwardDistance = 0;
    int totalDist = 0;
    bool foundNextSensor = false;
    auto forward = &startNode;
    while (!(foundNextSensor && forwardDistance >= model.trains.get(train).stoppingDistance)) {
        auto dir = DIR_AHEAD;
        if (forward->type == NODE_BRANCH) {
            if (model.switches[forward->num] == 'C')
                dir = DIR_CURVED;
        }
        if (!reserve(train, forward - Track.nodes, totalDist, forward->edge[dir].dist)) return false;
        totalDist += forward->edge[dir].dist;
        if (foundNextSensor) {
            forwardDistance += forward->edge[dir].dist;
        }
        if (forward->edge[dir].dest->type == NODE_SENSOR) {
            foundNextSensor = true;
        }
        forward = forward->edge[dir].dest;
    }

    return true;
}

bool Reservations::hasReservation(Train train, NodeIdx idx) const {
    const TrainReservation &tr = trainReservations.get(train);
    for (Size i = 0; i < tr.length; i++) {
        // TODO: time-based hasReservation
        if (tr.reservations[i].node == idx) {
            return true;
        }
    }
    return false;
}
