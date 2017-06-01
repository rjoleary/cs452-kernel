#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
void Scheduler::readyProcess(Td &td) {
    int pri = td.pri.underlying();
    auto priBit = 1u << pri;
    // There are other ready processes for this priority.
    // TODO Might not be a good builtin expect
    if (__builtin_expect(status & priBit, 0)) {
        entries[pri].last->nextReady = &td;
        entries[pri].last = &td;
    }
    else {
        entries[pri].first =
            entries[pri].last = &td;
        status |= priBit;
        if (lowestTask < pri) lowestTask = pri;
    }
    td.nextReady = nullptr;
    td.state = RunState::Ready;
}

Td* Scheduler::getNextProcess() {
    if (__builtin_expect(status == 0, 0)) return nullptr;
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
