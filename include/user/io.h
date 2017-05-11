// Input/Output

#ifndef USER_IO_H__INCLUDED
#define USER_IO_H__INCLUDED

#include "types.h"

// Getc - get a character from a UART.
//
// Description:
//   Getc returns next unreturned character from the given UART. The first
//   argument is the task id of the appropriate server.
//
//   How communication errors are handled is implementation-dependent.
//
//   Getc is actually a wrapper for a send to the appropriate server.
int Getc(Tid tid, int uart);

// Putc - transmit a character from the given UART.
//
// Description:
//   Putc queues the given character for transmission by the given UART. On
//   return the only guarantee is that the character has been queued. Whether it
//   has been transmitted or received is not guaranteed.
//
//   How configuration errors are handled is implementation-dependent.
//
//   Putc is actually a wrapper for a send to the serial server.
//
// Returns:
//   0 Success.
//   -1 The server task id is not the task id of an existing task.
int Putc(Tid tid, int uart, char ch);

#endif // USER_IO_H__INCLUDED
