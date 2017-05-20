#include <console.h>
#include <def.h>
#include <panic.h>
#include <scheduler.h>
#include <syscall.h>
#include <task.h>
#include <user/bwio.h>
#include <user/err.h>

// Forward decls
const char* buildstr(void);
void firstMain(void);

// First task must be created by hand.
void initFirstTask(struct Td *td, void *stack) {
    td->tid = 0;
    td->ptid = td->tid;
    td->pri = 3;
    td->nextReady = 0;
    td->sendReady = 0;
    td->state = READY;
    td->sp = stack;
}

__attribute__((section("user_stacks"))) static unsigned userStacks[NUM_TD][STACK_SZ/4];

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    bwprintf(COM2, BEGIN_SYS_CL "  [-] %s\r\n" END_CL, buildstr());

    unsigned used_tds = 1;
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initFirstTask(&tds[0], userStacks[0] + STACK_SZ/4);
    initStack(firstMain, tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &kernel_entry;

    while (1) {
        struct Td* active = getNextProcess(&scheduler);
        if (!active) {
            break;
        }
        active->sp = kernel_exit(active->sp);
        switch (reqSyscall(active)) {
            case SYS_CREATE: {
                Priority priority = reqArg(active, 0);
                void *code = (void*)reqArg(active, 1);
                int ret;
                if (used_tds == NUM_TD) {
                    ret = -ERR_NORES;
                } else if (priority < 0 || 31 < priority) {
                    ret = -ERR_BADARG;
                } else {
                    tds[used_tds].tid       = used_tds;
                    tds[used_tds].ptid      = active->tid;
                    tds[used_tds].pri       = priority;
                    tds[used_tds].nextReady = 0;
                    tds[used_tds].sendReady = 0;
                    tds[used_tds].state     = READY;
                    tds[used_tds].sp        = userStacks[used_tds] + STACK_SZ/4;
                    initStack(code, tds[used_tds].sp);
                    readyProcess(&scheduler, &tds[used_tds]);
                    ret = used_tds;
                    used_tds++;
                }
                reqSetReturn(active, ret);
                readyProcess(&scheduler, active);
                bwprintf(COM2, BEGIN_SYS_CL "  [%d] int create(priority=%d, code=%d) = %d\r\n" END_CL,
                        active->tid, priority, code, ret);
                break;
            }

            case SYS_MYTID: {
                int ret = active->tid;
                readyProcess(&scheduler, active);
                reqSetReturn(active, ret);
                bwprintf(COM2, BEGIN_SYS_CL "  [%d] int myTid() = %d\r\n" END_CL, active->tid, ret);
                break;
            }

            case SYS_MYPARENTTID: {
                int ret = active->ptid;
                readyProcess(&scheduler, active);
                reqSetReturn(active, ret);
                bwprintf(COM2, BEGIN_SYS_CL "  [%d] int myParentTid() = %d\r\n" END_CL, active->tid, ret);
                break;
            }

            case SYS_PASS: {
                readyProcess(&scheduler, active);
                bwprintf(COM2, BEGIN_SYS_CL "  [%d] void pass()\r\n" END_CL, active->tid);
                break;
            }

            case SYS_EXEUNT: {
                active->state = ZOMBIE;
                bwprintf(COM2, BEGIN_SYS_CL "  [%d] void exeunt()\r\n" END_CL, active->tid);
                break;
            }
            /* TODO: Implement in kernel 2.
            case SYS_DESTROY: {
                bwprintf(COM2, "void destroy()\r\n");
                // TODO
                break;
            }

            case SYS_SEND: {
                bwprintf(COM2, "int send(tid=, msg=, msglen=, reply=, rplen=) = \r\n"); // TODO: args
                // TODO
                break;
            }

            case SYS_RECEIVE: {
                bwprintf(COM2, "int receive(tid=, msg=, msglen=) = \r\n"); // TODO: args
                // TODO
                break;
            }

            case SYS_REPLY: {
                bwprintf(COM2, "int reply(tid=, reply=, rplen=) = \r\n"); // TODO: args
                // TODO
                break;
            }

            case SYS_AWAITEVENT: {
                bwprintf(COM2, "int awaitEvent(eventid=) = \r\n"); // TODO: args
                // TODO
                break;
            }
            */

            default: {
                PANIC("bad syscall number");
            }
        }
    }

    bwprintf(COM2, BEGIN_SYS_CL "  [-] No active tasks, returning\r\n" END_CL);
    return 0;
}
