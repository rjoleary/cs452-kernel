#include <scheduler.h>
#include <td.h>

void initScheduler(struct Scheduler *scheduler) {
    scheduler->status = 0;
}

void readyProcess(struct Scheduler *scheduler, struct Td *td) {
    unsigned int priBit = 1 << td->pri;
    // There are other ready processes for this priority
    if (scheduler->status & priBit) {
        scheduler->entries[priBit].last->nextReady = td;
        scheduler->entries[priBit].last = td;
    }
    else {
        scheduler->entries[priBit].first =
            scheduler->entries[priBit].last = td;
        scheduler->status |= priBit;
    }
}

struct Td* getNextProcess(struct Scheduler *scheduler) {
    if (scheduler->status == 0) return 0;
    unsigned int lowestPri = 31 - __builtin_clz(scheduler->status);
    struct Td* ready = scheduler->entries[lowestPri].first;
    // More than just one entry for this pri
    if (scheduler->entries[lowestPri].last != ready) {
        scheduler->entries[lowestPri].first = ready->nextReady;
    }
    else {
        scheduler->status &= ~lowestPri;
    }
    return ready;
}
