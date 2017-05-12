// Interrupt processing

#ifndef USER_INT_H__INCLUDED
#define USER_INT_H__INCLUDED

#include "syscall.h"

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
int awaitEvent(int eventid) SYSCALL(SYS_AWAITEVENT)

#endif // USER_INT_H__INCLUDED
