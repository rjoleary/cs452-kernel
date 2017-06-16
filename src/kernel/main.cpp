#include <def.h>
#include <err.h>
#include <interrupt.h>
#include <panic.h>
#include <profiler.h>
#include <scheduler.h>
#include <strace.h>
#include <syscall.h>
#include <task.h>
#include <user/math.h>
#include <user/std.h>
#include <user/ts7200.h>

using namespace kernel;
using namespace ctl;


// Forward declarations
const char* buildstr();

// The attribute allows us to specify the exact location of the user stacks in
// the linker script. This will be useful when it comes to memory protection.
__attribute__((section("user_stacks"))) 
static unsigned userStacks[NUM_TD][STACK_SZ/4];

static int copyMsg(const unsigned *src, int srcSize, unsigned *dest, int destSize) {
    int ret;
    if (destSize < srcSize) ret = -static_cast<int>(ctl::Error::Trunc);
    else ret = srcSize;

    fast_memcpy(dest, src, ctl::min(srcSize, destSize));
    return ret;
}

void initCaches() {
#ifdef CACHE_ENABLED
    asm volatile (
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "orr r0, r0, #1<<12\n\t"
        "orr r0, r0, #1<<2\n\t"
        "mcr p15, 0, r0, c1, c0, 0"
    );
#endif // CACHE_ENABLED
}

void initSerial() {
    // COM1
    bwsetspeed(COM1, 2400);
    bwsetfifo(COM1, OFF);

    // COM2
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    *(volatile unsigned*)(UART2_BASE + UART_CTLR_OFFSET) = UARTEN_MASK /*| RIEN_MASK*/;
}

void printEarlyDebug(unsigned *kernelStack) {
    // Print the build string (date + time).
    bwprintf(COM2, "Build string: '%s'\r\n", buildstr());

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
}

void initTimers() {
    // Setup TIMER1:
    // - 16-bit precision
    // - 2 kHz clock
    // - timer starts at 19 and counts down
    // - interrupt occurs every 10ms
    *(volatile unsigned*)(TIMER1_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER1_BASE + LDR_OFFSET) = 19;
    *(volatile unsigned*)(TIMER1_BASE + CLR_OFFSET) = 0;
    *(volatile unsigned*)(TIMER1_BASE + CRTL_OFFSET) = ENABLE_MASK | MODE_MASK;

    // Setup TIMER2:
    // - 16-bit precision
    // - 508 kHz clock
    // - timer starts at 0xffff and counts down
    // - It is always expected the timer will be reset before underflowing.
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER2_BASE + LDR_OFFSET) = 0xffff;
    *(volatile unsigned*)(TIMER2_BASE + CLR_OFFSET) = 0;
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = CLKSEL_MASK | ENABLE_MASK;
}

void initSwi() {
    // Software Interrupt (SWI)
    auto SVC_ADDR = (void (*volatile*)())0x28;
    *SVC_ADDR = &kernelEntry;
}

void initProfiler() {
#ifdef PROF_INTERVAL
    profilerStart(PROF_INTERVAL);
#endif // PROF_INTERVAL
}

int main() {
    // Approximately the starting address of the kernel stack.
    unsigned *kernelStack = (unsigned*)(&kernelStack + 1);

    // Initialize subsystems
    initCaches();
    initSerial();
    printEarlyDebug(kernelStack);
    initTimers();
    initProfiler();
    initSwi();

    // Kernel state
    Scheduler scheduler;
    TdManager tdManager(scheduler, userStacks[0] + STACK_SZ/4);
    InterruptController intControl; // enables interrupts

    // Enter main loop.
    //useBusyWait = false;
    void mainLoop(Scheduler &scheduler, TdManager &tdManager, InterruptController &intControl);
    mainLoop(scheduler, tdManager, intControl);
    useBusyWait = true;

#ifdef PROF_INTERVAL
    profilerStop();
#endif // PROF_INTERVAL

    bwputstr(COM2, "\r\n");
    tdManager.printUsage();

#ifdef PROF_INTERVAL
    profilerDump();
#endif

    return 0;
}

void mainLoop(Scheduler &scheduler, TdManager &tdManager, InterruptController &intControl) {
    while (1) {
        auto active = scheduler.getNextTask();
        *(volatile unsigned*)(TIMER2_BASE + LDR_OFFSET) = 0xffff;
        active->sp = kernelExit(active->sp);
        active->userTime += 0xffff - *(volatile unsigned*)(TIMER2_BASE + VAL_OFFSET);
        *(volatile unsigned*)(TIMER2_BASE + LDR_OFFSET) = 0xffff;

        // Handle interrupt.
        extern unsigned isIrq;
        if (isIrq) {
            isIrq = 0;
            intControl.handle(scheduler, tdManager);

            // Reschedule interrupted task.
            active->interruptLinkReg();
            scheduler.readyTask(*active);
            continue;
        }

        // Handle syscall.
        if (active->getSyscall() == Syscall::Receive) {
            using ctl::Error;
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

                scheduler.readyTask(*active);
                STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                        active->tid, senderMsg, senderMsglen, msg, msglen);
                STRACE("  [%d] Received from %d", active->tid, *tid);
            }
            else {
                active->state = kernel::RunState::ReceiveBlocked;
                STRACE("  [%d] ReceiveBlocked", active->tid);
            }
        }

        else if (active->getSyscall() == Syscall::Reply) {
            int ret = 0;
            auto tid = ctl::Tid(active->getArg(0));
            auto reply = (const unsigned*)active->getArg(1);
            int rplen = active->getArg(2);
            STRACE("  [%d] int reply(tid=%d, reply=0x%08x, rplen=%d)",
                    active->tid, tid, reply, rplen);
            auto receiver = tdManager.getTd(tid);
            if (__builtin_expect(!receiver, 0)) {
                ret = -static_cast<int>(Error::InvId);
                STRACE("  [%d] Bad Receiver", active->tid);
            }

            else if (__builtin_expect(receiver->state != kernel::RunState::ReplyBlocked, 0)) {
                ret = -static_cast<int>(Error::BadItc);
                STRACE("  [%d] Receiver not ReplyBlocked", active->tid);
            }
            else {
                auto receiverMsg = (unsigned*)receiver->getArg(3);
                int receiverMsglen = receiver->getArg(4);

                auto size = copyMsg(reply, rplen, receiverMsg, receiverMsglen);
                receiver->setReturn(size);
                scheduler.readyTask(*receiver);
                STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                        active->tid, reply, rplen, receiverMsg, receiverMsglen);
                    STRACE("  [%d] Replied to %d", active->tid, receiver->tid);
            }
            active->setReturn(ret);
            scheduler.readyTask(*active);
        }
        else if (active->getSyscall() == Syscall::Send) {
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
            if (__builtin_expect(!receiver, 0)) {
                active->setReturn(-static_cast<int>(Error::InvId));
                scheduler.readyTask(*active);
                STRACE("  [%d] Bad Receiver", active->tid);
            }
            else if (receiver->state == kernel::RunState::ReceiveBlocked) {
                auto recTid = (ctl::Tid*)receiver->getArg(0);
                auto recMsg = (unsigned*)receiver->getArg(1);
                int recMsglen = receiver->getArg(2);

                *recTid = active->tid;

                receiver->setReturn(copyMsg(msg, msglen, recMsg, recMsglen));

                scheduler.readyTask(*receiver);
                active->state = kernel::RunState::ReplyBlocked;
                STRACE("  [%d] SendMsg: %x %d ReceiveMsg %x %d",
                        active->tid, msg, msglen, recMsg, recMsglen);
                STRACE("  [%d] ReplyBlocked", active->tid);
            }
            else {
                receiver->pushSender(*active);
                STRACE("  [%d] SendBlocked", active->tid);
            }
        }
        else if (active->getSyscall() == Syscall::AwaitEvent) {
            auto eventId = (ctl::Event)active->getArg(0);
            int ret = intControl.awaitEvent(active, eventId);
            if (ret == 0) {
                STRACE("  [%d] int awaitEvent(eventid=%d) = <BLOCKED>", active->tid, eventId);
            } else {
                active->setReturn(ret);
                scheduler.readyTask(*active);
                STRACE("  [%d] int awaitEvent(eventid=%d) = %d", active->tid, eventId, ret);
            }
        }
        else if (active->getSyscall() == Syscall::Create) {
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
                td->ptid = active->tid;
                td->pri  = priority;
                td->sp   = userStacks[td->tid.underlying()] + STACK_SZ/4;
                td->initStack(code);
                scheduler.readyTask(*td);
                ret = td->tid.underlying();
            }
            active->setReturn(ret);
            scheduler.readyTask(*active);
            STRACE("  [%d] int create(priority=%d, code=%d) = %d",
                    active->tid, priority, code, ret);
        }

        else if (active->getSyscall() == Syscall::MyTid) {
            int ret = active->tid.underlying();
            scheduler.readyTask(*active);
            active->setReturn(ret);
            STRACE("  [%d] int myTid() = %d", active->tid, ret);
        }

        else if (active->getSyscall() == Syscall::MyParentTid) {
            int ret = active->ptid.underlying();
            scheduler.readyTask(*active);
            active->setReturn(ret);
            STRACE("  [%d] int myParentTid() = %d", active->tid, ret);
        }

        else if (active->getSyscall() == Syscall::Pass) {
            scheduler.readyTask(*active);
            if (!(active->tid == ctl::IDLE_TID)) {
                STRACE("  [%d] void pass()", active->tid);
            }
        }

        else if (active->getSyscall() == Syscall::Exeunt) {
            active->state = kernel::RunState::Zombie;
            STRACE("  [%d] void exeunt()", active->tid);

            // Quit when the active task exits.
            if (active->tid == ctl::FIRST_TID) {
                break;
            }
        }

        // TODO: Implement in kernel 4?
        //case SYS_DESTROY: {
        //    STRACE("  [%d] void destroy()", active->tid);
        //    // TODO
        //    break;
        //}

        else {
            PANIC("bad syscall number");
        }

        // Only sys time for syscalls is recorded. Time spent processing
        // interrupts is ignored.
        active->sysTime += 0xffff - *(volatile unsigned*)(TIMER2_BASE + VAL_OFFSET);
    }
}
