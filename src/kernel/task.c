#include <def.h>
#include <task.h>

void initTds(struct Td *tds) {
    for (int i = 0; i < NUM_TD; i++) {
        tds[i].tid = -1;
    }
}

struct Td* getTdByTid(struct Td *tds, Tid tid) {
    // TODO: may be inefficient
    for (int i = 0; i < NUM_TD; i++) {
        if (tds[i].tid == tid) {
            return &tds[i];
        }
    }
    return 0;
}

// All tasks start with this stub. It enforces calling exeunt when the task
// returns. This function always runs in usermode.
void taskStub() {
    register void (*entrypoint)() asm("r4");
    entrypoint();
    exeunt();
}

void initStack(const void *entrypoint, void **sp) {
    unsigned *word = *sp;
    *(--word) = (unsigned)taskStub; // r15/pc
    // no r12 or r13
    *(--word) = 0; // r11/fp
    *(--word) = 0; // r10/sl
    *(--word) = 0; // r9
    *(--word) = 0; // r8
    *(--word) = 0; // r7
    *(--word) = 0; // r6
    *(--word) = 0; // r5
    *(--word) = (unsigned)entrypoint; // r4
    // TODO: process status register
    *sp = (void*)word;
}
