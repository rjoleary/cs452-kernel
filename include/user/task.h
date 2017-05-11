// Task creation.

#ifndef USER_TASK_H__INCLUDED
#define USER_TASK_H__INCLUDED

#include "types.h"

// Create - instantiate a task.
//
// Description:
//   Create allocates and initializes a task descriptor, using the given
//   priority, and the given function pointer as a pointer to the entry point
//   of executable code, essentially a function with no arguments and no return
//   value. When Create returns the task descriptor has all the state needed to
//   run the task, the task's stack has been suitably initialized, and the task
//   has been entered into its ready queue so that it will run the next time it
//   is scheduled.
//
// Returns:
//   tid: the positive integer task id of the newly created task. The task id
//   is unique in the sense that no task has, will have or has had the same
//   task id.
Tid Create(Priority priority, void (*code)());

// MyTid - return my task id.
//
// Description:
//   MyTid returns the task id of the calling task.
//
// Returns:
//   tid: the positive integer task id of the task that calls it.
Tid MyTid();

// MyParentTid - return the task id of the task that created the calling task.
//
// Description:
//   MyParentTid returns the task id of the task that created the calling task.
//   This will be problematic only if the task has exited or been destroyed, in
//   which case the return value is implementation-dependent.
//
// Returns:
//   tid: the task id of the task that created the calling task
Tid MyParentTid();

// Pass - cease execution, remaining ready to run
//
// Description:
//   Pass causes a task to stop executing. The task is moved to the end of its
//   priority queue, and will resume executing when next sheduled.
void Pass();

// Exit - terminate execution forever.
//
// Description:
//   Exit causes a task to cease execution permanently. It is removed from all
//   priority queues, send queues, receive queues and awaitEvent queues.
//   Resources owned by the task, primarily its memory and task descriptors are
//   not reclaimed.
//
// Returns:
//   Exit does not return. If a point occurs where all tasks have exited the
//   kernel should return cleanly to RedBoot.
void Exit();

// Destroy
// TODO:  Please see the separate document for Destroy. Re-using resources is complicated.

#endif // USER_TASK_H__INCLUDED