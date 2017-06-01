// Interrupt processing

#ifndef USER_EVENT_H__INCLUDED
#define USER_EVENT_H__INCLUDED

namespace ctl {
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

// awaitEvent - wait for an external event.
//
// Description:
//   awaitEvent blocks until the event identified by eventid occurs then
//   returns, with volatile data, if any.
//
// Returns:
//   >-1: volatile data, in the form of a positive integer.
//   -ERR_INVID: invalid event.
//   -ERR_CORRUPT: corrupted volatile data.
int awaitEvent(InterruptSource eventid);
}

#endif // USER_EVENT_H__INCLUDED
