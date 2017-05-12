// Input/Output

#ifndef USER_IO_H__INCLUDED
#define USER_IO_H__INCLUDED

#include "types.h"

// getc - get a character from a UART.
//
// Description:
//   getc returns next unreturned character from the given UART. The first
//   argument is the task id of the appropriate server.
//
//   How communication errors are handled is implementation-dependent.
//
//   getc is actually a wrapper for a send to the appropriate server.
int getc(Tid tid, int uart);

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
//   ERR_OK: Success.
//   ERR_INVID: The server task id is not the task id of an existing task.
int putc(Tid tid, int uart, char ch);

#endif // USER_IO_H__INCLUDED
