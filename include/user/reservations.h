#pragma once

#include "cache.h"
#include "model.h"
#include "path_finding.h"
#include "sensor.h"
#include "std.h"
#include "train.h"

// Keeps track of reservations for the model layer.
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

    const ModelState &model;
    Cache<MAX_CONCURRENT_TRAINS, Train, TrainReservation> trainReservations;
    bool reserve(Train train, NodeIdx node);
    bool sensorTriggered(Train train, Sensor sensor);
  public:
    Reservations(const ModelState &model);
    void printReservations() const;
    // Return true if there is no contention.
    void doReservations(Train train, Sensor sensor, Speed speed);
    bool hasReservation(Train, NodeIdx) const;
};
