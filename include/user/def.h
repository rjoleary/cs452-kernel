#ifndef USER_DEF_H__INCLUDED
#define USER_DEF_H__INCLUDED

#include "types.h"

// The total number of task descriptors. The number of task descriptors is
// fixed so that lookup and creating new tasks is constant time.
constexpr auto NUM_TD = 32;

constexpr auto NUM_RPS_CLIENTS = 5;

static_assert(NUM_TD > NUM_RPS_CLIENTS, "Too many RPS clients");

namespace ctl {
constexpr Tid FIRST_TID{0},
          NS_TID{1},
          INVALID_TID{-1};
constexpr Priority FIRST_PRI{2};
};

#endif // USER_DEF_H__INCLUDED
