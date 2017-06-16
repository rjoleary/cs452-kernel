#pragma once

#include <user/syscall.h>

extern "C" {
// kernelEntry is registered as the interrupt handler.
void kernelEntry(void);

// kernelExit pops a tasks state from the given task pointer and context
// switches to it. The new stack pointer is returned.
unsigned * kernelExit(void *sp);
}
