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
        Size reservationLen;
        NodeIdx nodes[15];
        Time enterTime[15];
        Time exitTime[15];
    };

    const ModelState &model_;
    Cache<MAX_CONCURRENT_TRAINS, Train, TrainReservation> reservations_;

  public:
    Reservations(const ModelState &model);
    void sensorTriggered(Train train, Sensor sensor);
};
