#ifndef SCHEDULER_H__INCLUDED
#define SCHEDULER_H__INCLUDED

struct Td;

struct Scheduler {
    unsigned int status;
    struct {
        struct Td *first, *last;
    } entries[32];
};

void initScheduler(struct Scheduler *scheduler);
// Put process into ready queue
void readyProcess(struct Scheduler *scheduler, struct Td *td);
// Get next ready process, or NULL if there is none
struct Td* getNextProcess(struct Scheduler *scheduler);

#endif //SCHEDULER_H__INCLUDED
