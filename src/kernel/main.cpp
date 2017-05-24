#include <def.h>
#include <err.h>
#include <panic.h>
#include <scheduler.h>
#include <strace.h>
#include <syscall.h>
#include <task.h>

// Forward declarations
const char* buildstr();
void firstMain();

// The attribute allows us to specify the exact location of the user stacks in
// the linker script. This will be useful when it comes to memory protection.
__attribute__((section("user_stacks"))) static unsigned userStacks[kernel::NUM_TD][kernel::STACK_SZ/4];

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
    STRACE("  [-]   0x%08x - 0x%08x: user stacks (%d)", &userStart, &userEnd, kernel::NUM_TD);
    STRACE("  [-] 0x%08x - 0x%08x: text", &textStart, &textEnd);
    STRACE("  [-] 0x%08x - 0x%08x: kernel stack", &textEnd, kernelStack);
#endif

    unsigned used_tds = 1;
    kernel::Td tds[kernel::NUM_TD];
    kernel::Scheduler scheduler;

    kernel::initFirstTask(tds[0], userStacks[0] + kernel::STACK_SZ/4);
    tds[0].initStack(firstMain);
    scheduler.readyProcess(tds[0]);

    auto SVC_VECTOR = (void (*volatile*)())0x28;
    *SVC_VECTOR = &kernelEntry;

    while (1) {
        auto active = scheduler.getNextProcess();
        if (!active) {
            break;
        }
        active->sp = kernelExit(active->sp);
        switch (active->getSyscall()) {
            case SYS_CREATE: {
                Priority priority = active->getArg(0);
                auto code = (void(*)())active->getArg(1);
                int ret;
                if (used_tds == kernel::NUM_TD) {
                    ret = -ERR_NORES;
                } else if (priority < 0 || 31 < priority) {
                    ret = -ERR_BADARG;
                } else {
                    tds[used_tds].tid       = used_tds;
                    tds[used_tds].ptid      = active->tid;
                    tds[used_tds].pri       = priority;
                    tds[used_tds].state     = kernel::RunState::Ready;
                    tds[used_tds].sp        = userStacks[used_tds] + kernel::STACK_SZ/4;
                    tds[used_tds].initStack(code);
                    scheduler.readyProcess(tds[used_tds]);
                    ret = used_tds;
                    used_tds++;
                }
                active->setReturn(ret);
                scheduler.readyProcess(*active);
                STRACE("  [%d] int create(priority=%d, code=%d) = %d",
                        active->tid, priority, code, ret);
                break;
            }

            case SYS_MYTID: {
                int ret = active->tid;
                scheduler.readyProcess(*active);
                active->setReturn(ret);
                STRACE("  [%d] int myTid() = %d", active->tid, ret);
                break;
            }

            case SYS_MYPARENTTID: {
                int ret = active->ptid;
                scheduler.readyProcess(*active);
                active->setReturn(ret);
                STRACE("  [%d] int myParentTid() = %d", active->tid, ret);
                break;
            }

            case SYS_PASS: {
                scheduler.readyProcess(*active);
                STRACE("  [%d] void pass()", active->tid);
                break;
            }

            case SYS_EXEUNT: {
                active->state = kernel::RunState::Zombie;
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
