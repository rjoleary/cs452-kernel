#pragma once

#include <def.h>
#include <user/std.h>

namespace kernel {
// Forward declaration
struct Td;

class Scheduler : private NonCopyable {
    // A bitset indicating which queues are non-empty.
    unsigned status = 0;

    int lowestTask = -1;

    // One singly linked list for each priority.
    struct {
        Td *first, *last;
    } entries[NUM_PRI];
public:
    // Enqueue a task onto the ready queue.
    void readyTask(Td &td);

    // Get next ready task. The task is removed from the ready queue and must
    // be reinserted with `readyTask`. Because the idle task is always ready,
    // this function always returns a valid id.
    Td* getNextTask();
};
}
