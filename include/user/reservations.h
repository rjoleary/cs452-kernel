#pragma once

#include "cache.h"
#include "safety.h"
#include "path_finding.h"
#include "sensor.h"
#include "std.h"
#include "train.h"

// Keeps track of reservations for the safety layer.
class Reservations {
    struct TrainReservation {
        Size length = 0;
        NodeIdx reservations[15];
    };

    struct Waitlist {
        Size length = 0;
        struct {
            Train train;
            Sensor sensor;
            Speed speed;
        } items[15];
    } waitlist;

    const SafetyState &safety_;
    Cache<MAX_CONCURRENT_TRAINS, Train, TrainReservation> trainReservations_;

    bool reserveNode(Train train, NodeIdx node);
    bool reserveForSensor(Train train, Sensor sensor);
  public:
    Reservations(const SafetyState &safety);
    void printReservations() const;
    // Return true if there is no contention.
    void processSensor(Train train, Sensor sensor, Speed speed);
    bool hasReservation(Train, NodeIdx) const;
};
