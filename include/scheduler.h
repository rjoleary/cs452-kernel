#ifndef SCHEDULER_H__INCLUDED
#define SCHEDULER_H__INCLUDED

#include <def.h>

// Forward declaration
struct Td;

struct Scheduler {
    // A bitset indicating which queues are non-empty.
    unsigned status;

    // One singly linked list for each priority.
    struct {
        struct Td *first, *last;
    } entries[NUM_PRI];
};

// Initialize a new scheduler.
void initScheduler(struct Scheduler *scheduler);

// Enqueue a process onto the ready queue.
void readyProcess(struct Scheduler *scheduler, struct Td *td);

// Get next ready process, or NULL if there is none. The process is removed
// from the ready queue and must be reinserted with `readyProcess`.
struct Td* getNextProcess(struct Scheduler *scheduler);

#endif // SCHEDULER_H__INCLUDED
