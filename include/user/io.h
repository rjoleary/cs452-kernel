// Input/Output
#pragma once

#include "err.h"
#include "types.h"

namespace io {
// getc - get a character from a UART.
//
// Description:
//   getc returns next unreturned character from the given UART. The first
//   argument is the task id of the appropriate server.
//
//   How communication errors are handled is implementation-dependent.
//
//   getc is actually a wrapper for a send to the appropriate server.
ctl::ErrorOr<int> getc(ctl::Tid tid);

// putc - transmit a character from the given UART.
//
// Description:
//   putc queues the given character for transmission by the given UART. On
//   return the only guarantee is that the character has been queued. Whether it
//   has been transmitted or received is not guaranteed.
//
//   How configuration errors are handled is implementation-dependent.
//
//   putc is actually a wrapper for a send to the serial server.
//
// Returns:
//   -ERR_OK: Success.
//   -ERR_INVID: The server task id is not the task id of an existing task.
ctl::ErrorOr<void> putc(ctl::Tid tid, char ch);

// flush - flush characters held in a per-task queue.
//
// Description:
//   putc does not write to the UART controller immediately. To ensure
//   atomicity, the io server implements per-task buffers. This functions
//   flushes the buffer for the current task.
//
//   Buffers are also flushed for two other reasons:
//
//     1. The buffer becomes full. This is 8 bytes for COM1 and 64 bytes for
//        COM2.
//     2. The newline character is printed. This only applies to COM2.
//
// Returns:
//   -ERR_OK: Success.
//   -ERR_INVID: The server task id is not the task id of an existing task.
ctl::ErrorOr<void> flush(ctl::Tid tid);
}
