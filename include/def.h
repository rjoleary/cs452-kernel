// This file contains constants global to the kernel.

#ifndef DEF_H__INCLUDED
#define DEF_H__INCLUDED

#include <user/def.h>

// Size of individual user stacks in bytes.
#ifndef STACK_SZ
#warning Stack size should be defined in Makefile.
#define STACK_SZ 4096
#endif

namespace kernel {

// The number of available priorities.
constexpr auto NUM_PRI = ctl::PRIORITY_MAX.underlying() - ctl::PRIORITY_MIN.underlying() + 1;
}

#endif // DEF_H__INCLUDED
