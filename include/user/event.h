// Interrupt processing
#pragma once

namespace ctl {
// Kernel event on which a usert task may wait.
enum class Event {
    // This timer fires every 10ms.
    // Input: ignored
    // Output: 0
    PeriodicTimer,

    // This event fires whenever data is received on Uart1.
    // Input: ignored
    // Output: 8-bit char or -Error::Corrupt
    Uart1Rx,

    // This event writes to Uart1 and blocks until the FIFO is empty.
    // Input: 8-bit char to write
    // Output: 0
    Uart1Tx,

    // This event fires whenever data is received on Uart2.
    // Input: ignored
    // Output: 8-bit char or -Error::Corrupt
    Uart2Rx,

    // This event writes to Uart2 and blocks until the FIFO is empty.
    // Input: 8-bit char to write
    // Output: 0
    Uart2Tx,

    // Number of events, keep as last.
    EventNum,
};

const int EVENT_NUM = static_cast<int>(Event::EventNum);

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
int awaitEvent(Event eventid);
}
