// Name server
#pragma once

#include <task.h>
#include <err.h>

namespace ctl{

// Names are capped at eight characters including the null terminator.
struct Name {char data[8];};

namespace names {
constexpr Name ClockServer{"Clock"};
constexpr Name Uart1RxServer{"Uart1Rx"};
constexpr Name Uart1TxServer{"Uart1Tx"};
constexpr Name Uart2RxServer{"Uart2Rx"};
constexpr Name Uart2TxServer{"Uart2Tx"};
}

// registerAs - register a name with the name server.
//
// Description
//   registerAs registers the task id of the caller under the given name.
//
//   On return without error it is guaranteed that all WhoIs calls by any task
//   will return the task id of the caller until the registration is
//   overwritten.
//
//   If another task has already registered with the given name its
//   registration is overwritten.
//
//   A single task may register under several different names, but each name is
//   assigned to a single task.
//
//   registerAs is actually a wrapper covering a send to the name server.
//
// Returns:
//   -ERR_OK: Success.
//   -ERR_INVID: The nameserver task id inside the wrapper is invalid.
ErrorOr<void> registerAs(Name name);

// whoIs - query the nameserver.
//
// Description:
//   whoIs asks the nameserver for the task id of the task that is registered
//   under the given name.
//
//   Whether whoIs blocks waiting for a registration or returns with an error
//   if no task is registered under the given name is implementation-dependent.
//
//   There is guaranteed to be a unique task id associated with each registered
//   name, but the registered task may change at any time after a call to
//   whoIs.
//
//   whoIs is actually a wrapper covering a send to the nameserver.
//
// Returns:
//   tid: The task id of the registered task.
//   -ERR_BADARG: Cannot find the name.
ErrorOr<Tid> whoIs(Name name);

// reverseWhoIs - query the nameserver.
//
// Description:
//   reverseWhoIs asks the nameserver for the names of the task that is
//   registered to the given task id.
//
//   reverseWhoIs is actually a wrapper covering a send to the nameserver.
//
// Returns:
//   -ERR_OK: Success
//   -ERR_BADARG: Cannot find the tid
ErrorOr<void> reverseWhoIs(Tid tid, Name *name);

void nsMain();
}
