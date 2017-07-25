#pragma once

#include "switch.h"
#include "train.h"

// Gradient Absolute Switch Profile
struct Gasp {
    SwitchStates gradient;
    Position end;
};
