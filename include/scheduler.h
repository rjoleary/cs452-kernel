#ifndef SCHEDULER_H__INCLUDED
#define SCHEDULER_H__INCLUDED

#include <def.h>

namespace kernel {
// Forward declaration
struct Td;

struct Scheduler {
    // A bitset indicating which queues are non-empty.
    unsigned status = 0;

    // One singly linked list for each priority.
    struct {
        Td *first, *last;
    } entries[NUM_PRI];

    // Enqueue a process onto the ready queue.
    void readyProcess(Td &td);

    // Get next ready process, or NULL if there is none. The process is removed
    // from the ready queue and must be reinserted with `readyProcess`.
    Td* getNextProcess();
};
}

#endif // SCHEDULER_H__INCLUDED
