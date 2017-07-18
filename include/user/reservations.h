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
        struct {
            NodeIdx node;
            Time start, end;
        } reservations[15];
    };

    const ModelState &model;
    Cache<MAX_CONCURRENT_TRAINS, Train, TrainReservation> trainReservations;
    bool reserve(Train train, NodeIdx node, int dist, int length);

  public:
    Reservations(const ModelState &model);
    // Return true if there is no contention.
    bool sensorTriggered(Train train, Sensor sensor);
    bool hasReservation(Train, NodeIdx) const;
};
