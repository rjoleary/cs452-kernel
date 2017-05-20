#ifndef SYSCALL_H__INCLUDED
#define SYSCALL_H__INCLUDED

#include <user/syscall.h>

// kernelEntry is registered as the interrupt handler.
void kernelEntry(void);

// kernelExit pops a tasks state from the given task pointer and context
// switches to it. The new stack pointer is returned.
unsigned * kernelExit(void *sp);

#endif // SYCALL_H__INCLUDED
