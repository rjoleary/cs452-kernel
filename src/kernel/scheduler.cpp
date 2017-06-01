#include <panic.h>
#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
void Scheduler::readyTask(Td &td) {
    unsigned pri = td.pri.underlying();
    auto priBit = 1u << pri;
    // There are other ready tasks for this priority.
    if (status & priBit) {
        entries[pri].last->nextReady = &td;
        entries[pri].last = &td;
    }
    else {
        entries[pri].first =
            entries[pri].last = &td;
        status |= priBit;
    }
    td.state = RunState::Ready;
}

Td* Scheduler::getNextTask() {
    if (status == 0) {
        // Should never happen because the idle task is always ready.
        PANIC("no ready tasks");
    }
    unsigned int lowestPri = 31 - __builtin_clz(status);
    auto ready = entries[lowestPri].first;
    // More than just one entry for this priority.
    if (entries[lowestPri].last != ready) {
        entries[lowestPri].first = ready->nextReady;
    }
    else {
        status &= ~(1 << lowestPri);
    }
    ready->state = RunState::Active;
    return ready;
}
}
