#ifndef INTERRUPT_H__INCLUDED
#define INTERRUPT_H__INCLUDED

#include <user/event.h>

// Initialize the vectored interrupt controller.
void initInterrupts();

// Bind an interrupt to a address. When the interrupt is triggered, control is
// transferred to that address. To return from the interrupt, run the
// instruction `subs pc, lr`. Interrupts are initially disabled.
void bindInterrupt(InterruptSource src, unsigned vector, void (*isr)());

// Enable/disable a specific interrupt.
void enableInterrupt(InterruptSource src);
void disableInterrupt(InterruptSource src);

#endif // INTERRUPT_H__INCLUDED
