#include <def.h>
#include <err.h>
#include <panic.h>
#include <scheduler.h>
#include <strace.h>
#include <syscall.h>
#include <task.h>
#include <user/math.h>
#include <user/std.h>

// Forward declarations
const char* buildstr();

// The attribute allows us to specify the exact location of the user stacks in
// the linker script. This will be useful when it comes to memory protection.
__attribute__((section("user_stacks"))) 
static unsigned userStacks[NUM_TD][kernel::STACK_SZ/4];

static int copyMsg(const unsigned *src, int srcSize, unsigned *dest, int destSize) {
    int ret;
    if (destSize < srcSize) ret = -static_cast<int>(ctl::Error::Trunc);
    else ret = srcSize;

    fast_memcpy(dest, src, ctl::min(srcSize, destSize));
    return ret;
}

int main() {
#ifdef CACHE_ENABLED
    asm volatile (
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "orr r0, r0, #1<<12\n\t"
        "orr r0, r0, #1<<2\n\t"
        "mcr p15, 0, r0, c1, c0, 0"
    );
#endif // CACHE_ENABLED

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
#endif // STRACE_ENABLED

    kernel::Scheduler scheduler;
    kernel::TdManager tdManager(scheduler, userStacks[0] + kernel::STACK_SZ/4);


    /*constexpr auto siz = 12;
    unsigned src[siz], dest[siz];
    for (int i = 0; i < siz; ++i) src[i] = i;
    fast_memcpy(dest,src,siz*4);
    for (int i = 0; i < siz; ++i)
        bwprintf(COM2, "mem %d %d\r\n", src[i], dest[i]);*/

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
                auto priority = ctl::Priority(active->getArg(0));
                auto code = (void(*)())active->getArg(1);
                int ret;
                kernel::Td* td;
                if (priority < ctl::PRIORITY_MIN || ctl::PRIORITY_MAX < priority) {
                    ret = -static_cast<int>(Error::BadArg);
                }
                else if (!(td = tdManager.createTd())) {
                    ret = -static_cast<int>(Error::NoRes);
                } 
                else {
                    td->ptid      = active->tid;
                    td->pri       = priority;
                    td->sp        = userStacks[td->tid.underlying()] + kernel::STACK_SZ/4;
                    td->initStack(code);
                    scheduler.readyProcess(*td);
                    ret = td->tid.underlying();
                }
                active->setReturn(ret);
                scheduler.readyProcess(*active);
                STRACE("  [%d] int create(priority=%d, code=%d) = %d",
                        active->tid, priority, code, ret);
                break;
            }

            case Syscall::MyTid: {
                int ret = active->tid.underlying();
                scheduler.readyProcess(*active);
                active->setReturn(ret);
                STRACE("  [%d] int myTid() = %d", active->tid, ret);
                break;
            }

            case Syscall::MyParentTid: {
                int ret = active->ptid.underlying();
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
                auto tid = ctl::Tid(active->getArg(0));
                auto msg = (const unsigned*)active->getArg(1);
                int msglen = active->getArg(2);
                auto reply = (unsigned*)active->getArg(3);
                (void)reply;
                int rplen = active->getArg(4);
                (void)rplen;
                auto receiver = tdManager.getTd(tid);
                STRACE("  [%d] int send(tid=%d, msg=0x%08x, msglen=%d, reply=0x%08x, rplen=%d)",
                        active->tid, tid, msg, msglen, reply, rplen);
                if (!receiver) {
                    active->setReturn(-static_cast<int>(Error::InvId));
                    scheduler.readyProcess(*active);
                    STRACE("  [%d] Bad Receiver", active->tid);
                }
                else if (receiver->state == kernel::RunState::ReceiveBlocked) {
                    auto recTid = (ctl::Tid*)receiver->getArg(0);
                    auto recMsg = (unsigned*)receiver->getArg(1);
                    int recMsglen = receiver->getArg(2);

                    *recTid = active->tid;

                    receiver->setReturn(copyMsg(msg, msglen, recMsg, recMsglen));

                    scheduler.readyProcess(*receiver);
                    active->state = kernel::RunState::ReplyBlocked;
                    STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                            active->tid, msg, msglen, recMsg, recMsglen);
                    STRACE("  [%d] ReplyBlocked", active->tid);
                }
                else {
                    receiver->pushSender(*active);
                    STRACE("  [%d] SendBlocked", active->tid);
                }
                break;
            }

            case Syscall::Receive: {
                auto tid = (ctl::Tid*)active->getArg(0);
                auto msg = (unsigned*)active->getArg(1);
                int msglen = active->getArg(2);
                STRACE("  [%d] int receive(tid=0x%08d, msg=0x%08x, msglen=%d)",
                        active->tid, tid, msg, msglen);
                auto sender = active->popSender();
                if (sender) {
                    auto senderMsg = (const unsigned*)sender->getArg(1);
                    int senderMsglen = sender->getArg(2);

                    *tid = sender->tid;

                    active->setReturn(copyMsg(senderMsg, senderMsglen, msg, msglen));

                    scheduler.readyProcess(*active);
                    STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                            active->tid, senderMsg, senderMsglen, msg, msglen);
                    STRACE("  [%d] Received from %d", active->tid, *tid);
                }
                else {
                    active->state = kernel::RunState::ReceiveBlocked;
                    STRACE("  [%d] ReceiveBlocked", active->tid);
                }
                break;
            }

            case Syscall::Reply: {
                int ret = 0;
                auto tid = ctl::Tid(active->getArg(0));
                auto reply = (const unsigned*)active->getArg(1);
                int rplen = active->getArg(2);
                STRACE("  [%d] int reply(tid=%d, reply=0x%08x, rplen=%d)",
                        active->tid, tid, reply, rplen);
                auto receiver = tdManager.getTd(tid);
                if (!receiver) {
                    ret = -static_cast<int>(Error::InvId);
                    STRACE("  [%d] Bad Receiver", active->tid);
                }
                else if (receiver->state != kernel::RunState::ReplyBlocked) {
                    ret = -static_cast<int>(Error::BadItc);
                    STRACE("  [%d] Receiver not ReplyBlocked", active->tid);
                }
                else {
                    auto receiverMsg = (unsigned*)receiver->getArg(3);
                    int receiverMsglen = receiver->getArg(4);

                    auto size = copyMsg(reply, rplen, receiverMsg, receiverMsglen);
                    receiver->setReturn(size);
                    scheduler.readyProcess(*receiver);
                    STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                            active->tid, reply, rplen, receiverMsg, receiverMsglen);
                    STRACE("  [%d] Replied to %d", active->tid, receiver->tid);
                }
                active->setReturn(ret);
                scheduler.readyProcess(*active);
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
