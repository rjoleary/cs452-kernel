#pragma once

#include "switch.h"
#include "model.h"

enum class SwitchGradient : U8 {
    DontCare,
    Branch1,
    Branch2,
    Merge,
};

// Gradient Absolute Switch Profile
struct Gasp {
    SwitchGradient gradient[NumSwitches];
    ModelServer::Position end;
};
