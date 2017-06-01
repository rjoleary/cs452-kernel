#ifndef INTERRUPT_H__INCLUDED
#define INTERRUPT_H__INCLUDED

#include <user/event.h>

namespace kernel {
// Initialize the vectored interrupt controller.
void initInterrupts();

// Bind an interrupt to a address. When the interrupt is triggered, control is
// transferred to that address. To return from the interrupt, run the
// instruction `subs pc, lr`. Interrupts are initially disabled.
void bindInterrupt(ctl::InterruptSource src, unsigned vector, void *isr);
int enableOnly(ctl::InterruptSource src, unsigned vector, void *isr);
void clearInterrupt(ctl::InterruptSource src, unsigned vector);

// Enable/disable a specific interrupt.
void enableInterrupt(ctl::InterruptSource src);
void disableInterrupt(ctl::InterruptSource src);
}

#endif // INTERRUPT_H__INCLUDED
