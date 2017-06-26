#pragma once

#include "types.h"

// The total number of task descriptors. The number of task descriptors is
// fixed so that lookup and creating new tasks is constant time.
constexpr auto NUM_TD = 32;

namespace ctl {
constexpr Tid FIRST_TID{0},
          NS_TID{1},
          IDLE_TID{2},
          INVALID_TID{-1};
constexpr Priority FIRST_PRI{10};
}
