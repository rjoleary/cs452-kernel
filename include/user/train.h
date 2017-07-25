// Functions for controlling the trains.
#pragma once

#include "int.h"
#include "types.h"
#include "path_finding.h"

using Train = ctl::NamedType<char, struct Train_t>;
constexpr Train INVALID_TRAIN{0};

const Train AnticipatedTrains[] = {
    Train(24),
    Train(58), Train(63), Train(69), Train(70),
    Train(71), Train(73), Train(74), Train(76),
};

// Representation of a position offset from a node.
struct Position {
    NodeIdx nodeIdx;
    Distance offset; // mm
};
constexpr Position INVALID_POSITION{INVALID_NODE, 0};

class TrainServer {
    ctl::Tid tid;
  public:
    TrainServer();

    void setupTrains();
    void stopTrains();
    void goTrains();

    // Inserts a delay before the next command.
    void addDelay(Train train, Time delay);

    // train: [1, 80]
    // speed: [0, 14]
    void setTrainSpeed(Train train, int speed);

    // train: [1, 80]
    void reverseTrain(Train train);
};
