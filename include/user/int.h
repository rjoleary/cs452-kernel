// Interrupt processing

#ifndef USER_INT_H__INCLUDED
#define USER_INT_H__INCLUDED

// awaitEvent - wait for an external event.
//
// Description:
//   awaitEvent blocks until the event identified by eventid occurs then
//   returns, with volatile data, if any.
//
// Returns:
//   >-1: volatile data, in the form of a positive integer.
//   -1: invalid event.
//   -2: corrupted volatile data.
int awaitEvent(int eventid);

#endif // USER_INT_H__INCLUDED
