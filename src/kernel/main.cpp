#include <def.h>
#include <err.h>
#include <panic.h>
#include <scheduler.h>
#include <strace.h>
#include <syscall.h>
#include <task.h>

// Forward declarations
const char* buildstr();

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

    kernel::Scheduler scheduler;
    kernel::TdManager tdManager(scheduler, userStacks[0] + kernel::STACK_SZ/4);

    auto SVC_VECTOR = (void (*volatile*)())0x28;
    *SVC_VECTOR = &kernelEntry;

    while (1) {
        auto active = scheduler.getNextProcess();
        if (!active) {
            break;
        }
        active->sp = kernelExit(active->sp);
        switch (active->getSyscall()) {
            using kernel::Syscall;
            using ctl::Error;
            case Syscall::Create: {
                Priority priority = active->getArg(0);
                auto code = (void(*)())active->getArg(1);
                int ret;
                kernel::Td* td;
                if (priority < 0 || 31 < priority) {
                    ret = -static_cast<int>(Error::BadArg);
                }
                else if (!(td = tdManager.createTd())) {
                    ret = -static_cast<int>(Error::NoRes);
                } 
                else {
                    td->ptid      = active->tid;
                    td->pri       = priority;
                    td->sp        = userStacks[td->tid] + kernel::STACK_SZ/4;
                    td->initStack(code);
                    scheduler.readyProcess(*td);
                    ret = td->tid;
                }
                active->setReturn(ret);
                scheduler.readyProcess(*active);
                STRACE("  [%d] int create(priority=%d, code=%d) = %d",
                        active->tid, priority, code, ret);
                break;
            }

            case Syscall::MyTid: {
                int ret = active->tid;
                scheduler.readyProcess(*active);
                active->setReturn(ret);
                STRACE("  [%d] int myTid() = %d", active->tid, ret);
                break;
            }

            case Syscall::MyParentTid: {
                int ret = active->ptid;
                scheduler.readyProcess(*active);
                active->setReturn(ret);
                STRACE("  [%d] int myParentTid() = %d", active->tid, ret);
                break;
            }

            case Syscall::Pass: {
                scheduler.readyProcess(*active);
                STRACE("  [%d] void pass()", active->tid);
                break;
            }

            case Syscall::Exeunt: {
                active->state = kernel::RunState::Zombie;
                STRACE("  [%d] void exeunt()", active->tid);
                break;
            }
            /* TODO: Implement in kernel 2.
            case SYS_DESTROY: {
                STRACE("  [%d] void destroy()", active->tid);
                // TODO
                break;
            }*/

            case Syscall::Send: {
                /*int tid = active->getArg(0);
                char *msg = active->getArg(1);
                int msglen = active->getArg(2);
                char *reply = active->getArg(3);
                int rplen = active->getArg(4);
                STRACE("  [%d] int send(tid=%d, msg=%d, msglen=%d, reply=%d, rplen=%d)",
                        active->tid, tid, msg, msglen, reply, rplen);*/
                break;
            }

            case Syscall::Receive: {
                STRACE("  [%d] int receive(tid=, msg=, msglen=) = ", active->tid); // TODO: args
                // TODO
                break;
            }

            case Syscall::Reply: {
                STRACE("  [%d] int reply(tid=, reply=, rplen=) = ", active->tid); // TODO: args
                // TODO
                break;
            }
            /*
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
