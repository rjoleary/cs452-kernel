// This file contains constants global to the kernel.

#ifndef DEF_H__INCLUDED
#define DEF_H__INCLUDED

namespace kernel {
// The total number of task descriptors. The number of task descriptors is
// fixed so that lookup and creating new tasks is constant time.
constexpr static auto NUM_TD = 32;

// Size of individual user stacks in bytes.
constexpr static auto STACK_SZ = 4096;

// The number of available priorities.
constexpr static auto NUM_PRI = 32;
}

#endif // DEF_H__INCLUDED
