#ifndef INTERRUPT_H__INCLUDED
#define INTERRUPT_H__INCLUDED

#include <user/event.h>

namespace kernel {
// Manual for the interrupt controller:
//     http://infocenter.arm.com/help/topic/com.arm.doc.ddi0181e/DDI0181.pdf
volatile char *const VIC1Base = (char *)(0x800b0000);
volatile char *const VIC2Base = (char *)(0x800c0000);
const unsigned
    VICxIRQStatus    = 0x00,
    VICxFIQStatus    = 0x04,
    VICxRawIntr      = 0x08,
    VICxIntSelect    = 0x0c,
    VICxIntEnable    = 0x10,
    VICxIntEnClear   = 0x14,
    VICxSoftInt      = 0x18,
    VICxSoftIntClear = 0x1c,
    VICxProtection   = 0x20,
    VICxVectAddr     = 0x30,
    VICxDefVectAddr  = 0x34,
    VICxVectAddr0    = 0x100,
    VICxVectCntl0    = 0x200;

// Initialize the vectored interrupt controller.
void initInterrupts();

// Uninitialize the vectored interrupt controller.
// Must be called to correctly return to RedBoot and re-execute the kernel.
void uninitInterrupts();

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
