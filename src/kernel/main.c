#include <def.h>
#include <err.h>
#include <panic.h>
#include <scheduler.h>
#include <strace.h>
#include <syscall.h>
#include <task.h>

// Forward declarations
const char* buildstr(void);
void firstMain(void);

// The attribute allows us to specify the exact location of the user stacks in
// the linker script. This will be useful when it comes to memory protection.
__attribute__((section("user_stacks"))) static unsigned userStacks[NUM_TD][STACK_SZ/4];

int main() {
    unsigned *kernelStack = (unsigned*)(&kernelStack + 1);

    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    STRACE("  [-] Built %s", buildstr());

    // Print memory layout.
#ifdef STRACE_ENABLED
    extern char _DataStart, _DataEnd, _BssStart, _BssEnd;
    extern char userStart, userEnd, textStart, textEnd;
    STRACE("  [-] Memory layout:");
    STRACE("  [-] 0x%08x - 0x%08x: data", &_DataStart, &_DataEnd);
    STRACE("  [-] 0x%08x - 0x%08x: bss", &_BssStart, &_BssEnd);
    STRACE("  [-]   0x%08x - 0x%08x: user stacks (%d)", &userStart, &userEnd, NUM_TD);
    STRACE("  [-] 0x%08x - 0x%08x: text", &textStart, &textEnd);
    STRACE("  [-] 0x%08x - 0x%08x: kernel stack", &textEnd, kernelStack);
#endif

    unsigned used_tds = 1;
    struct Td tds[NUM_TD];
    struct Scheduler scheduler;

    initTds(tds);
    initFirstTask(&tds[0], userStacks[0] + STACK_SZ/4);
    initStack(firstMain, tds[0].sp);
    initScheduler(&scheduler);
    readyProcess(&scheduler, &tds[0]);

    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &kernelEntry;

    while (1) {
        struct Td* active = getNextProcess(&scheduler);
        if (!active) {
            break;
        }
        active->sp = kernelExit(active->sp);
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
                STRACE("  [%d] int create(priority=%d, code=%d) = %d",
                        active->tid, priority, code, ret);
                break;
            }

            case SYS_MYTID: {
                int ret = active->tid;
                readyProcess(&scheduler, active);
                reqSetReturn(active, ret);
                STRACE("  [%d] int myTid() = %d", active->tid, ret);
                break;
            }

            case SYS_MYPARENTTID: {
                int ret = active->ptid;
                readyProcess(&scheduler, active);
                reqSetReturn(active, ret);
                STRACE("  [%d] int myParentTid() = %d", active->tid, ret);
                break;
            }

            case SYS_PASS: {
                readyProcess(&scheduler, active);
                STRACE("  [%d] void pass()", active->tid);
                break;
            }

            case SYS_EXEUNT: {
                active->state = ZOMBIE;
                STRACE("  [%d] void exeunt()", active->tid);
                break;
            }
            /* TODO: Implement in kernel 2.
            case SYS_DESTROY: {
                STRACE("  [%d] void destroy()", active->tid);
                // TODO
                break;
            }

            case SYS_SEND: {
                STRACE("  [%d] int send(tid=, msg=, msglen=, reply=, rplen=) = ", active->tid); // TODO: args
                // TODO
                break;
            }

            case SYS_RECEIVE: {
                STRACE("  [%d] int receive(tid=, msg=, msglen=) = ", active->tid); // TODO: args
                // TODO
                break;
            }

            case SYS_REPLY: {
                STRACE("  [%d] int reply(tid=, reply=, rplen=) = ", active->tid); // TODO: args
                // TODO
                break;
            }

            case SYS_AWAITEVENT: {
                STRACE("  [%d] int awaitEvent(eventid=) = ", active->tid); // TODO: args
                // TODO
                break;
            }
            */

            default: {
                PANIC("bad syscall number");
            }
        }
    }

    STRACE("  [-] No active tasks, returning");
    return 0;
}
