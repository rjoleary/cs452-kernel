#pragma once

#include "switch.h"
#include "train.h"

// Gradient Absolute Switch Profile
struct Gasp {
    SwitchState gradient;
    Position end;
    Speed speed;
};
