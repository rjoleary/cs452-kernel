// This file contains global definitions.

#ifndef DEF_H__INCLUDED
#define DEF_H__INCLUDED

// The total number of task descriptors. The number of task descriptors is
// fixed because they are stored on the kernel stack.
#define NUM_TD 32

// Size of individual user stacks.
#define STACK_SZ 4096

#endif // DEF_H__INCLUDED
