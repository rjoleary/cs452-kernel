// Functions for controlling the trains.
#pragma once

#include "int.h"
#include "types.h"
#include "path_finding.h"

using Train = ctl::NamedType<char, struct Train_t>;
constexpr Train INVALID_TRAIN{0};

// Representation of a position offset from a node.
struct Position {
    NodeIdx nodeIdx;
    Distance offset; // mm

};
inline bool operator==(const Position &lhs, const Position &rhs) {
    return lhs.nodeIdx == rhs.nodeIdx && lhs.offset == rhs.offset;
}

constexpr Position INVALID_POSITION = {(NodeIdx)-1, -1};

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
