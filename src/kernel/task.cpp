#include <def.h>
#include <panic.h>
#include <task.h>
#include <user/bwio.h>
#include <scheduler.h>

// Forward declerations
namespace ctl {
void firstMain();
}

namespace kernel {
namespace {
// All tasks start with this stub. It enforces calling exeunt when the task
// returns. This function always runs in usermode.
void taskStub(void (*entrypoint)(void)) {
    entrypoint();
    ctl::exeunt();
}
} // unnamed namespace

TdManager::TdManager(Scheduler &scheduler, unsigned *stack) {
    tds[0].tid = tds[0].ptid = ctl::FIRST_TID;
    tds[0].pri = ctl::FIRST_PRI;
    tds[0].state = kernel::RunState::Ready;
    tds[0].sp = stack;
    tds[0].initStack(ctl::firstMain);
    scheduler.readyProcess(tds[0]);
}

Td* TdManager::createTd() {
    if (usedTds == NUM_TD) return nullptr;
    tds[usedTds].tid.underlying() = usedTds;
    return &tds[usedTds++];
}

Td* TdManager::getTd(ctl::Tid tid) {
    return &tds[tid.underlying()];
    // TODO: may be inefficient
    for (int i = 0; i < NUM_TD; i++) {
        if (tds[i].tid == tid) {
            return &tds[i];
        }
    }
    return nullptr;
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

Td* Td::popSender() {
    auto ret = sendBegin;
    if (sendBegin) {
        sendBegin = sendBegin->nextReady;
        ret->state = RunState::ReplyBlocked;
    }
    return ret;
}

void Td::pushSender(Td &sender) {
    if (__builtin_expect(!sendBegin, 1)) {
        sendBegin = 
            sendEnd = &sender;
    }
    else {
        sendEnd->nextReady = &sender;
        sendEnd = &sender;
    }
    sender.nextReady = nullptr;
    sender.state = RunState::SendBlocked;
}
}
