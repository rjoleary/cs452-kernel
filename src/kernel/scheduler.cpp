#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
void Scheduler::readyProcess(Td &td) {
    unsigned int priBit = 1 << td.pri;
    // There are other ready processes for this priority.
    if (status & priBit) {
        entries[td.pri].last->nextReady = &td;
        entries[td.pri].last = &td;
    }
    else {
        entries[td.pri].first =
            entries[td.pri].last = &td;
        status |= priBit;
    }
    td.state = RunState::Ready;
}

Td* Scheduler::getNextProcess() {
    if (status == 0) return 0;
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
