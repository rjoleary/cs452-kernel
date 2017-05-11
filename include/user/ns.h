// Name server

#ifndef USER_NS_H__INCLUEDED
#ifndef USER_NS_H__INCLUEDED

// RegisterAs - register a name with the name server.
//
// Description
//   RegisterAs registers the task id of the caller under the given name.
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
//   RegisterAs is actually a wrapper covering a send to the name server.
//
// Returns:
//   0: Success.
//   -1: The nameserver task id inside the wrapper is invalid.
int RegisterAs(char *name);

// WhoIs - query the nameserver.
//
// Description:
//   WhoIs asks the nameserver for the task id of the task that is registered under the given name.
//
//   Whether WhoIs blocks waiting for a registration or returns with an error
//   if no task is registered under the given name is implementation-dependent.
//
//   There is guaranteed to be a unique task id associated with each registered
//   name, but the registered task may change at any time after a call to
//   WhoIs.
//
//   WhoIs is actually a wrapper covering a send to the nameserver.
//
// Returns:
//   tid: The task id of the registered task.
//   -1: The nameserver task id inside the wrapper is invalid.
int WhoIs(char *name);

#endif // USER_NS_H__INCLUEDED
