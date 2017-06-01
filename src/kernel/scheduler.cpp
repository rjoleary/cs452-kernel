#include <panic.h>
#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
void Scheduler::readyTask(Td &td) {
    unsigned pri = td.pri.underlying();
    auto priBit = 1u << pri;
    // There are other ready tasks for this priority.
    // TODO Might not be a good builtin expect
    if (__builtin_expect(status & priBit, 0)) {
        entries[pri].last->nextReady = &td;
        entries[pri].last = &td;
    }
    else {
        entries[pri].first =
            entries[pri].last = &td;
        status |= priBit;
        if (lowestTask == -1 || unsigned(lowestTask) < pri) lowestTask = pri;
    }
    td.nextReady = nullptr;
    td.state = RunState::Ready;
}

Td* Scheduler::getNextTask() {
    if (__builtin_expect(status == 0, 0)) {
        // Should never happen because the idle task is always ready.
        PANIC("no ready tasks");
    }
    unsigned int lowestPri;
    if (lowestTask != -1) {
        lowestPri = lowestTask;
    }
    else {
        lowestPri = 31 - __builtin_clz(status);
    }
    auto ready = entries[lowestPri].first;
    // More than just one entry for this priority.
    if (__builtin_expect(ready->nextReady != nullptr, 0)) {
        entries[lowestPri].first = ready->nextReady;
    }
    else {
        status &= ~(1 << lowestPri);
        lowestTask = -1;
    }
    ready->state = RunState::Active;
    return ready;
}
}
