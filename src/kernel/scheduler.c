#include <scheduler.h>
#include <task.h>
#include <user/bwio.h>

void initScheduler(struct Scheduler *scheduler) {
    scheduler->status = 0;
}

void readyProcess(struct Scheduler *scheduler, struct Td *td) {
    unsigned int priBit = 1 << td->pri;
    // There are other ready processes for this priority
    if (scheduler->status & priBit) {
        scheduler->entries[td->pri].last->nextReady = td;
        scheduler->entries[td->pri].last = td;
    }
    else {
        scheduler->entries[td->pri].first =
            scheduler->entries[td->pri].last = td;
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
        scheduler->status &= ~(1 << lowestPri);
    }
    return ready;
}
