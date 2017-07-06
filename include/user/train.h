// Functions for controlling the trains.
#pragma once

#include "types.h"

using Train = ctl::NamedType<char, struct Train_t>;
constexpr Train INVALID_TRAIN(0);

class TrainServer {
    ctl::Tid tid;
  public:
    TrainServer();

    void stopTrains();
    void goTrains();

    // train: [1, 80]
    void cmdToggleLight(Train train);

    // train: [1, 80]
    // speed: [0, 14]
    void cmdSetSpeed(Train train, int speed);

    // train: [1, 80]
    void cmdReverse(Train train);
};
