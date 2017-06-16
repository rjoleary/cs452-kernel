// Exports facilities for tasks.
#pragma once

#include "types.h"
#include "def.h"
#include "user/task.h"
#include "user/syscall.h"
#include "user/event.h"
#include "user/std.h"

namespace kernel {
class Scheduler;

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

const char interruptToStr[] = "ARZBBBE";

// Task descriptor
struct Td {
    constexpr static auto SyscallIndex = 5;
    ctl::Tid tid = ctl::INVALID_TID; // task id
    ctl::Tid ptid;                   // parent's task id
    ctl::Priority pri;               // priority
    Td *nextReady = nullptr;         // next TD in the ready queue, or NULL
                                     // reused for the interrupt queue
    Td *sendBegin = nullptr,         // queue for senders
       *sendEnd;
    RunState state;                  // current run state
    unsigned *sp;                    // current stack pointer
    unsigned long long userTime = 0; // number of 508kHz ticks spent in this task
    unsigned long long sysTime = 0;  // number of 508kHz ticks spent in the kernel

    // Return the syscall number for the given task descriptor. If the task is not
    // performing a syscall, garbage is returned.
    inline Syscall getSyscall() const {
        return static_cast<Syscall>(sp[-16 + SyscallIndex]);
    }

    // Return the sycall's i-th argument for the given task descriptor. If the task
    // is not performing a syscall, garbage is returned.
    inline unsigned getArg(int i) const {
        return sp[-16 + i];
    }

    // Set the syscall's return value for the given task descriptor. If the task is
    // not performing a syscall, its stack gets corrupted.
    inline void setReturn(unsigned v) {
        sp[-16] = v;
    }

    // Decrement the link register to the previous instruction. Required in
    // the cases of an interrupt.
    inline void interruptLinkReg() {
        sp[-1] -= 4;
    }

    // Set the initial state of a user's stack.
    void initStack(void (*entrypoint)());
    
    Td* popSender();
    void pushSender(Td&);
};

class TdManager : private NonCopyable {
    typename ctl::Tid::underlying_type usedTds{1};
    Td tds[NUM_TD];
public:
    TdManager(Scheduler &scheduler, unsigned *stack);
    // Create a Td, or return nullptr if impossible
    Td* createTd();

    // Return a Td or nullptr if Td does not exist.
    // Runtime: O(n)
    Td* getTd(ctl::Tid tid);
};
}
