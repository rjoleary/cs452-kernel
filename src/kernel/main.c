#include <bwio.h>
#include <def.h>
#include <panic.h>
#include <td.h>
#include <user/task.h>
#include <syscall.h>
#include <scheduler.h>

// Forward decls
const char* buildstr(void);
void firstMain(void);

// All tasks start with this stub. It enforces calling exeunt when the task
// returns. This function always runs in usermode.
// TODO: use
void taskStub(void (*entrypoint)()) {
    entrypoint();
    exeunt();
}

// First task must be created by hand.
void initFirstTask(struct Td *td, void *stack) {
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

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    bwprintf(COM2, "%s\r\n", buildstr());

    unsigned userStacks[NUM_TD][STACK_SZ/4];
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initFirstTask(&tds[0], userStacks[0] + STACK_SZ/4 - 1);
    initStack(firstMain, &tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &kernel_entry;

    struct Td* active = getNextProcess(&scheduler);
    unsigned ret;
    while (1) {
        bwprintf(COM2, "Context switching to TID %d\r\n", active->tid);
        bwprintf(COM2, "Context switching to sp 0x%08x\r\n", active->sp);
        enum Syscall syscall = kernel_exit(&active->sp);
        bwprintf(COM2, "SYSCALL %x\r\n", syscall);
        switch (syscall) {
            case SYS_CREATE:
                bwprintf(COM2, "int create(priority=, code=) = \r\n"); // TODO: args
                // TODO
                break;
            case SYS_MYTID:
                bwprintf(COM2, "int myTid() = \r\n"); // TODO: args
                ret = active->tid;
                break;
            case SYS_MYPARENTTID:
                bwprintf(COM2, "int myParentTid() = \r\n"); // TODO: args
                ret = active->ptid;
                break;
            case SYS_PASS:
                bwprintf(COM2, "void pass()\r\n");
                // TODO
                break;
            case SYS_EXEUNT:
                bwprintf(COM2, "void exeunt()\r\n");
                // TODO
                PANIC("Task exited");
                break;
            case SYS_DESTROY:
                bwprintf(COM2, "void destroy()\r\n");
                // TODO
                break;
            case SYS_SEND:
                bwprintf(COM2, "int send(tid=, msg=, msglen=, reply=, rplen=) = \r\n"); // TODO: args
                // TODO
                break;
            case SYS_RECEIVE:
                bwprintf(COM2, "int receive(tid=, msg=, msglen=) = \r\n"); // TODO: args
                // TODO
                break;
            case SYS_REPLY:
                bwprintf(COM2, "int reply(tid=, reply=, rplen=) = \r\n"); // TODO: args
                // TODO
                break;
            case SYS_AWAITEVENT:
                bwprintf(COM2, "int awaitEvent(eventid=) = \r\n"); // TODO: args
                // TODO
                break;
            default:
                PANIC("bad syscall number");
        }
    }

    return 0;
}
