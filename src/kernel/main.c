#include <bwio.h>
#include <def.h>
#include <panic.h>
#include <td.h>
#include <user/task.h>
#include <syscall.h>
#include <scheduler.h>
#include <task.h>
#include <itc.h>
#include <event.h>

// Forward decls
const char* buildstr(void);
void initMain(void);

// First task must be created by hand.
void initTask1(struct Td *td, void *stack) {
    td->tid = 1;
    td->ptid = 1;
    td->pri = 3;
    td->nextReady = 0;
    td->sendReady = 0;
    td->state = READY;
    td->sp = stack;
}

void initStack(const void *entrypoint, void **sp) {
    unsigned *word = *sp;
    *(word++) = 0; // r0
    *(word++) = 0; // r1
    *(word++) = 0; // r2
    *(word++) = 0; // r3
    *(word++) = 0; // r4
    *(word++) = 0; // r5
    *(word++) = 0; // r6
    *(word++) = 0; // r7
    *(word++) = 0; // r8
    *(word++) = 0; // r9
    *(word++) = 0; // r10/sl
    *(word++) = 0; // r11/fp
    *(word++) = 0; // r12/ip
    *(word++) = (unsigned)exeunt; // r14/lr
    *(word++) = (unsigned)entrypoint; // r15/pc
    // TODO: process status register
    *sp = (void*)word;
}

int svcHandle(unsigned id) {
    // Maps syscall number to kernel function.
    // TODO: make const static
    void *jumpTable[10];
    jumpTable[SYS_CREATE]     = kCreate;
    jumpTable[SYS_MYTID]      = kMyTid;
    jumpTable[SYS_MYPARENTID] = kMyParentTid;
    jumpTable[SYS_PASS]       = kPass;
    jumpTable[SYS_EXEUNT]     = kExeunt;
    jumpTable[SYS_DESTROY]    = kDestroy;
    jumpTable[SYS_SEND]       = kSend;
    jumpTable[SYS_RECEIVE]    = kReceive;
    jumpTable[SYS_REPLY]      = kReply;
    jumpTable[SYS_AWAITEVENT] = kAwaitEvent;

    bwprintf(COM2, "HANDLE %x\r\n", id);
    if (id >= SYS_NUM) { // TODO: signedness?
        PANIC("bad syscall number");
    }
    int (*f)(void) = jumpTable[id];
    int r = f();
    bwprintf(COM2, "RETURN %d\r\n", r);
    return r;
}

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    bwprintf(COM2, "%s\r\n", buildstr());

    static char userStacks[NUM_TD][STACK_SZ];
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initTask1(&tds[0], userStacks[0]);
    initStack(initMain, &tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &kernel_entry;

    while (1) {
        struct Td* active = getNextProcess(&scheduler);
        bwprintf(COM2, "Context switching to TID %d\r\n", active->tid);
        kernel_exit(active->sp);
    }

    return 0;
}
