// Exports facilities for task descriptors.

#ifndef TD_H__INCLUDED
#define TD_H__INCLUDED

#include "types.h"

// The first three of these states are needed for task creation; the next three
// are needed for message passing; and the seventh is needed for hardware
// interrupts.
enum RunState {
    // The task that has just run, is running, or is about to run. Scheduling,
    // which happens near the end of kernel processing, changes the active
    // task. On a single processor only one task can be active at a time.
    ACTIVE = 0,

    // The task is ready to be activated.
    READY,

    // The task will never again run, but still retains its resources: memory,
    // TD, etc.
    ZOMBIE,

    // The task has executed Receive, and is waiting for a task to send to it.
    SEND_BLOCKED,

    // The task has executed Send, and is waiting for the message to be
    // received.
    RECEIVE_BLOCKED,

    // The task have executed Send and its message has been received, but it
    // has not received a reply.
    REPLY_BLOCKED,

    // The task has executed AwaitEvent, but the event on which it is waiting
    // has not occurred.
    EVENT_BLOCKED,
};

// Task descriptor
struct Td {
    Tid tid;              // task id (-1 if td is unallocated)
    Tid ptid;             // parent's task id
    Priority pri;         // priority 0 to 31
    struct Td *nextReady; // next TD in the ready queue, or NULL
    struct Td *sendReady; // next TD in the send queue, or NULL
    enum RunState state;  // current run state
    void *stack;          // current stack pointer
};

// Initialize task descriptors.
void initTds(struct Td *tds);

// Return a Td or NULL if Td does not exist.
// Runtime: O(n)
struct Td* getTdByTid(struct Td *tds, Tid tid);

#endif // TD_H__INCLUDED
