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

void initStack(const void *entrypoint, void **sp) {
    unsigned *word = *sp;
    *(--word) = (unsigned)entrypoint; // r15/pc
    //*(--word) = (unsigned)exeunt; // r14/lr
    // no r12 or r13
    *(--word) = 0; // r11/fp
    *(--word) = 0; // r10/sl
    *(--word) = 0; // r9
    *(--word) = 0; // r8
    *(--word) = 0; // r7
    *(--word) = 0; // r6
    *(--word) = 0; // r5
    *(--word) = 0; // r4
    // TODO: process status register
    *sp = (void*)word;
}
