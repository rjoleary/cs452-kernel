// Exports facilities for tasks.

#ifndef TASK_H__INCLUDED
#define TASK_H__INCLUDED

#include "types.h"
#include "user/task.h"

namespace kernel {
// The first three of these states are needed for task creation; the next three
// are needed for message passing; and the seventh is needed for hardware
// interrupts.
enum class RunState {
    // The task that has just run, is running, or is about to run. Scheduling,
    // which happens near the end of kernel processing, changes the active
    // task. On a single processor only one task can be active at a time.
    Active = 0,

    // The task is ready to be activated.
    Ready,

    // The task will never again run, but still retains its resources: memory,
    // TD, etc.
    Zombie,

    // The task has executed Receive, and is waiting for a task to send to it.
    SendBlocked,

    // The task has executed Send, and is waiting for the message to be
    // received.
    ReceiveBlocked,

    // The task have executed Send and its message has been received, but it
    // has not received a reply.
    ReplyBlocked,

    // The task has executed AwaitEvent, but the event on which it is waiting
    // has not occurred.
    EventBlocked,
};

// Task descriptor
struct Td {
    Tid tid = -1;         // task id (-1 if td is unallocated)
    Tid ptid;             // parent's task id
    Priority pri;         // priority 0 to 31
    Td *nextReady;        // next TD in the ready queue, or NULL
    Td *sendReady;        // next TD in the send queue, or NULL
    enum RunState state;  // current run state
    unsigned *sp;         // current stack pointer

    // Return the syscall number for the given task descriptor. If the task is not
    // performing a syscall, garbage is returned.
    unsigned getSyscall() const {
        return sp[-16 + 9];
    }

    // Return the sycall's i-th argument for the given task descriptor. If the task
    // is not performing a syscall, garbage is returned.
    unsigned getArg(int i) const {
        return sp[-16 + i];
    }

    // Set the syscall's return value for the given task descriptor. If the task is
    // not performing a syscall, its stack gets corrupted.
    void setReturn(unsigned v) {
        sp[-16] = v;
    }
    // Set the initial state of a user's stack.
    void initStack(void (*entrypoint)());
};

// Return a Td or NULL if Td does not exist.
// Runtime: O(n)
struct Td* getTdByTid(Td *tds, Tid tid);

// The first task descriptor must be created by hand.
void initFirstTask(Td &td, unsigned *stack);
}

#endif // TASK_H__INCLUDED
