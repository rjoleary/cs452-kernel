#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>
namespace {
int clz(unsigned x)
{
    static constexpr char debruijn32[32] = {
        0, 31, 9, 30, 3, 8, 13, 29, 2, 5, 7, 21, 12, 24, 28, 19,
        1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18
    };
    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    x++;
    return debruijn32[x*0x076be629>>27];
}
}

namespace kernel {
void Scheduler::readyProcess(Td &td) {
    unsigned pri = td.pri.underlying();
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
        if (lowestTask->pri < td.pri) lowestTask = &td;
    }
    td.nextReady = nullptr;
    //td.state = RunState::Ready;
}

Td* Scheduler::getNextProcess() {
    if (__builtin_expect(status == 0, 0)) return nullptr;
    if (__builtin_expect(lowestTask != nullptr, 1)) {
        auto ret = lowestTask;
        lowestTask = lowestTask->nextReady;
        return ret;
    }
    unsigned int lowestPri = 31 - clz(status);
    auto ready = entries[lowestPri].first;
    // More than just one entry for this priority.
    if (ready->nextReady) {
        entries[lowestPri].first = ready->nextReady;
        lowestTask = ready->nextReady;
    }
    else {
        status &= ~(1 << lowestPri);
        lowestTask = nullptr;
    }
    //ready->state = RunState::Active;
    return ready;
}
}
