#include <user/bwio.h>
#include <def.h>
#include <panic.h>
#include <task.h>
#include <syscall.h>
#include <scheduler.h>

// Forward decls
const char* buildstr(void);
void firstMain(void);
void testMain(void); // TODO: remove

// First task must be created by hand.
void initFirstTask(struct Td *td, void *stack) {
    td->tid = 0;
    td->ptid = 0;
    td->pri = 3;
    td->nextReady = 0;
    td->sendReady = 0;
    td->state = READY;
    td->sp = stack;
}

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    bwprintf(COM2, "%s\r\n", buildstr());

    // TODO: user tasks should not be on the kernel stack!
    unsigned userStacks[NUM_TD][STACK_SZ/4];
    unsigned used_tds = 1;
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initFirstTask(&tds[0], userStacks[0] + STACK_SZ/4 - 1);
    initStack(firstMain, &tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &kernel_entry;

    unsigned ret;
    while (1) {
        struct Td* active = getNextProcess(&scheduler);
        struct Request req;
        bwprintf(COM2, "Context switching to TID %d\r\n", active->tid);
        enum Syscall syscall = kernel_exit(&active->sp, &req);
        bwprintf(COM2, "Request: %08x %08x %08x %08x %08x", req.a[0], req.a[1],
            req.a[2], req.a[3], req.a[4]);
        switch (syscall) {
            case SYS_CREATE: {
                // TODO: arguments are hard-coded atm.
                Priority priority = req.a[0];
                void *code = (void*)req.a[1];
                bwprintf(COM2, "int create(priority=%d, code=%d) = ", priority, code);
                if (used_tds == NUM_TD) {
                    ret = -2;
                } else if (priority < 0 || 31 < priority) {
                    ret = -1;
                } else {
                    tds[used_tds].tid       = used_tds;
                    tds[used_tds].ptid      = active->tid;
                    tds[used_tds].pri       = priority;
                    tds[used_tds].nextReady = 0;
                    tds[used_tds].sendReady = 0;
                    tds[used_tds].state     = READY;
                    tds[used_tds].sp        = userStacks[used_tds];
                    initStack(testMain, &tds[used_tds].sp);
                    readyProcess(&scheduler, &tds[used_tds]);
                    ret = used_tds;
                    used_tds++;
                }
                readyProcess(&scheduler, active);
                bwprintf(COM2, "%d\r\n", ret);
                break;
            }

            case SYS_MYTID: {
                bwprintf(COM2, "int myTid() = ");
                ret = active->tid;
                readyProcess(&scheduler, active);
                bwprintf(COM2, "%d\r\n", ret);
                break;
            }

            case SYS_MYPARENTTID: {
                bwprintf(COM2, "int myParentTid() = ");
                ret = active->ptid;
                readyProcess(&scheduler, active);
                bwprintf(COM2, "%d\r\n", ret);
                break;
            }

            case SYS_PASS: {
                bwprintf(COM2, "void pass()\r\n");
                readyProcess(&scheduler, active);
                break;
            }

            case SYS_EXEUNT: {
                bwprintf(COM2, "void exeunt()\r\n");
                active->state = ZOMBIE;
                break;
            }

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

            default:
                PANIC("bad syscall number");
        }
        unsigned *sp = (unsigned *)active->sp;
        *(--sp) = ret;
        active->sp = sp;
    }

    return 0;
}
