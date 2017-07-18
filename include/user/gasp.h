#pragma once

#include "switch.h"
#include "train.h"

enum class SwitchGradient : U8 {
    DontCare,
    Branch1,
    Branch2,
    Merge,
};

// Gradient Absolute Switch Profile
struct Gasp {
    SwitchState gradient;
    Position end;
};
