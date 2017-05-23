#include <def.h>
#include <panic.h>
#include <task.h>
#include <user/bwio.h>

namespace kernel {
Td* getTdByTid(Td *tds, Tid tid) {
    // TODO: may be inefficient
    for (int i = 0; i < kernel::NUM_TD; i++) {
        if (tds[i].tid == tid) {
            return &tds[i];
        }
    }
    return 0;
}

void initFirstTask(Td &td, unsigned *stack) {
    td.tid = td.ptid = 0;
    td.pri = 3;
    td.state = kernel::RunState::Ready;
    td.sp = stack;
}

// All tasks start with this stub. It enforces calling exeunt when the task
// returns. This function always runs in usermode.
void taskStub(void (*entrypoint)(void)) {
    entrypoint();
    exeunt();
}

void Td::initStack(void (*entrypoint)()) {
    // Note that the task's state is stored above the stack, in the location
    // which will be written to next according the the ABI. However, we do not
    // bother decrementing the stack pointer.
    // Also, the initially unused registers are numbered. This makes it easy to
    // tell if the registers have been misaligned during a context switch.
    unsigned *next = sp;
    *(--next) = (unsigned)taskStub; // r15/pc
    *(--next) = 0; // r14/lr
    *(--next) = (unsigned)sp; // r13/sp
    *(--next) = 12; // r12/ip
    *(--next) = 11; // r11/fp
    *(--next) = 10; // r10/sl
    *(--next) = 9; // r9, syscall number
    *(--next) = 8; // r8
    *(--next) = 7; // r7
    *(--next) = 6; // r6
    *(--next) = 5; // r5
    *(--next) = 4; // r4
    *(--next) = 3; // r3
    *(--next) = 2; // r2
    *(--next) = 1; // r1
    *(--next) = (unsigned)entrypoint; // r0
    *(--next) = 0x10; // cpsr
}
}
