#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
void Scheduler::readyProcess(Td &td) {
    int pri = td.pri.underlying();
    auto priBit = 1 << pri;
    // There are other ready processes for this priority.
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

Td* Scheduler::getNextProcess() {
    if (status == 0) return nullptr;
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
