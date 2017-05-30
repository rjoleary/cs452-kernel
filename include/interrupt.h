#ifndef INTERRUPT_H__INCLUDED
#define INTERRUPT_H__INCLUDED

// Interrupt sources which may be relevant.
// Source: ep93xx-user-guid.pdf, section 6.1.2
enum class InterruptSource {
    COMMRX       = 2,  // ARM Communication Rx for Debug
    COMMTX       = 3,  // ARM Communication Tx for Debug
    TC1UI        = 4,  // TC1 under flow interrupt (Timer Counter 1)
    TC2UI        = 5,  // TC2 under flow interrupt (Timer Counter 2)
    UART1RXINTR1 = 23, // UART 1 Receive Interrupt
    UART1TXINTR1 = 24, // UART 1 Transmit Interrupt
    UART2RXINTR2 = 25, // UART 2 Receive Interrupt
    UART2TXINTR2 = 26, // UART 2 Transmit Interrupt
    TC3UI        = 51, // TC3 under flow interrupt (Timer Counter 3)
    INT_UART1    = 52, // UART 1 Interrupt
    INT_UART2    = 54, // UART 2 Interrupt
};

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
