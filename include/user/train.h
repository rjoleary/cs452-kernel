// Functions for controlling the trains.
#pragma once

#include "int.h"
#include "types.h"

using Train = ctl::NamedType<char, struct Train_t>;
constexpr Train INVALID_TRAIN(0);

// TODO: make more strongly typed
typedef I32 Speed;
typedef I32 Velocity;
typedef I32 Distance;
typedef I32 Time;

// Representation of a position offset from a switch.
struct Position {
    int nodeIdx; // TODO: make NodeIdx typedef global
    Distance offset; // mm
};

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
