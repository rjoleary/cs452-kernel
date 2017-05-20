// This file contains constants global to the kernel.

#ifndef DEF_H__INCLUDED
#define DEF_H__INCLUDED

// The total number of task descriptors. The number of task descriptors is
// fixed so that lookup and creating new tasks is constant time.
#define NUM_TD 32

// Size of individual user stacks in bytes.
#define STACK_SZ 4096

// The number of available priorities.
#define NUM_PRI 32

#endif // DEF_H__INCLUDED
