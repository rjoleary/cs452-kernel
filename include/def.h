// This file contains constants global to the kernel.
#pragma once

#include <user/def.h>

// Size of individual user stacks in bytes.
static_assert(STACK_SZ, "Stack size should be defined in Makefile.");

namespace kernel {

// The number of available priorities.
constexpr auto NUM_PRI = ctl::PRIORITY_MAX.underlying() - ctl::PRIORITY_MIN.underlying() + 1;
}
