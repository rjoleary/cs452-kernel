#include <def.h>
#include <task.h>
#include <panic.h>
#include <user/bwio.h>
#include <console.h>

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
void taskStub(void (*entrypoint)(void)) {
    entrypoint();
    exeunt();
}

void initStack(const void *entrypoint, void *sp) {
    unsigned *word = sp;
    *(--word) = (unsigned)taskStub; // r15/pc
    *(--word) = 0; // r14/lr
    *(--word) = (unsigned)sp; // r13/sp
    *(--word) = 12; // r12/ip
    *(--word) = 11; // r11/fp
    *(--word) = 10; // r10/sl
    *(--word) = 9; // r9, syscall number
    *(--word) = 8; // r8
    *(--word) = 7; // r7
    *(--word) = 6; // r6
    *(--word) = 5; // r5
    *(--word) = 4; // r4
    *(--word) = 3; // r3
    *(--word) = 2; // r2
    *(--word) = 1; // r1
    *(--word) = (unsigned)entrypoint; // r0
    *(--word) = 0x10; // cpsr
}

unsigned reqSyscall(struct Td *td) {
    return ((unsigned*)td->sp)[-16 + 9];
}

unsigned reqArg(struct Td *td, int i) {
    return ((unsigned*)td->sp)[-16 + i];
}

void reqSetReturn(struct Td *td, unsigned v) {
    ((unsigned*)td->sp)[-16] = v;
}
