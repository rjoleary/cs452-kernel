// Clock server
#pragma once

#include "types.h"
#include "err.h"

namespace ctl {
// delay - wait for a given amount of time.
//
// Description:
//   delay returns after the given number of ticks has elapsed. How long after
//   is not guaranteed because the caller may have to wait on higher priority
//   tasks.
//
//   delay is (almost) identical to pass if ticks is zero or negative.
//
//   The size of a tick is normally application dependent. In CS452 this term
//   it is 10 milliseconds.
//
//   delay is actually a wrapper for a send to the clock server.
//
// Returns:
//   -ERR_OK: Success.
//   -ERR_INVID: The clock server task id is invalid.
//   -ERR_BADARG: The delay was zero or negative.
ErrorOr<void> delay(Tid tid, int ticks);

// time - give the time since clock server start up.
//
// Description:
//   time returns the number of ticks since the clock server was created and
//   initialized.
//
//   With a 10 millisecond tick and a 32-bit unsigned int for the time
//   wraparound is almost a million hours, plenty of time for your demo.
//
//   time is actually a wrapper for a send to the clock server. The argument is
//   the tid of the clock server.
//
// Returns:
//   >-1: The time in ticks since the clock server initialized.
//   -ERR_INVID: The clock server task id is invalid.
ErrorOr<int> time(Tid tid);

// delayUntil - wait until a time.
//
// Description:
//   delay returns when the time since clock server intialization is greater
//   than the given number of ticks. How long after is not guaranteed because
//   the caller may have to wait on higher priority tasks.
//
//   delayUntil(tid, Time(tid) + ticks) may differ from delay(tid, ticks) by a
//   small amount.
//
//   The size of a tick is normally application dependent. In CS452 this term
//   it is 10 milliseconds, the time in which a train at top speed travels
//   about 5 millimetres.
//
//   delayUntil is actually a wrapper for a send to the clock server.
//
// Returns;
//   -ERR_OK: Success.
//   -ERR_INVID: The clock server task id is invalid.
//   -ERR_BADARG: The delay was zero or negative.
ErrorOr<void> delayUntil(Tid tid, int ticks);
}
