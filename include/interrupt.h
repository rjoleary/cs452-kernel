#pragma once
#include <user/event.h>

namespace kernel {
// Forward declare
struct Td;

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

namespace interrupt {
// Interrupt sources which may be relevant.
// Source: ep93xx-user-guid.pdf, section 6.1.2
enum SourceVic1 {
    //COMMRX       = 2,  // ARM Communication Rx for Debug
    //COMMTX       = 3,  // ARM Communication Tx for Debug
    TC1UI        = 4,  // TC1 under flow interrupt (Timer Counter 1)
    TC2UI        = 5,  // TC2 under flow interrupt (Timer Counter 2)
    //UART1RXINTR1 = 23, // UART 1 Receive Interrupt
    //UART1TXINTR1 = 24, // UART 1 Transmit Interrupt
    //UART2RXINTR2 = 25, // UART 2 Receive Interrupt
    //UART2TXINTR2 = 26, // UART 2 Transmit Interrupt
};
enum SourceVic2 {
    TC3UI        = 51 - 32, // TC3 under flow interrupt (Timer Counter 3)
    INT_UART1    = 52 - 32, // UART 1 Interrupt
    INT_UART2    = 54 - 32, // UART 2 Interrupt
};

// Initialize the interrupt controller.
void init();

// Uninitialize the interrupt controller.
// Must be called to correctly return to RedBoot and re-execute the kernel.
void clearAll();
}

}
