#include <bwio.h>
#include <def.h>
#include <event.h>
#include <std.h>
#include <task.h>

#ifndef PERF_TEST

// Forward declaration.
void idleMain();
void rpsServerMain();
void rpsClientMain();

namespace ctl {
void nsMain();

void firstMain() {
    ASSERT(create(PRIORITY_MIN, idleMain) == IDLE_TID);
    ASSERT(create(PRIORITY_MAX, nsMain) == NS_TID);

    // Clock.
    for (int i = 0; ; i++) {
        bwprintf(COM2, "\r%d", i);
        awaitEvent(static_cast<int>(InterruptSource::TC1UI));
    }
}
}

#else

#include <ts7200.h>
#include <itc.h>
#include <../profiler.h>

namespace ctl {

template <size_t I>
struct Message {
    unsigned data[I/4];
};

template <size_t I>
void perfMain() {
    while (1) {
        Tid tid;
        Message<I> msg;
        receive(&tid, msg);
        reply(tid, msg);
    }
}

template <size_t I, int P>
void testThing() {
    constexpr auto AmountToSend = 10'000;//, ClockTicks = 508'000, Microseconds = 1'000'000;
    auto timerVal = (volatile unsigned*)(TIMER3_BASE + VAL_OFFSET);
    bwprintf(COM2, "Size %d Pri %d...\r\n", I, P);
    asm volatile ("":::"memory"); // memory barrier
    auto start = *timerVal;
    Tid tid = Tid(create(Priority(P), perfMain<I>));

    // Start profiler.
    //profilerStart(1000);

    // Actual test.
    Message<I> msg;
    for (unsigned i = 0; i < AmountToSend; i++) {
        send(tid, msg, msg);
    }

    //profilerStop();

    unsigned elapsed = start - *timerVal; 
    asm volatile ("":::"memory"); // memory barrier
    bwprintf(COM2, "Done! Elapsed: %u\r\n", elapsed);

    // Dump profiler.
    //profilerDump();
    bwputstr(COM2, "\r\n");
}

void firstMain() {
    bwputstr(COM2, "Running performance test...\r\n");

    // Setup TIMER1:
    // - 32-bit precision
    // - 508 kHz clock
    // - each 1ms is 508 ticks
    // - timer starts at 0xffffffff and counts down
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER3_BASE + LDR_OFFSET) = 0xffffffffU;
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = ENABLE_MASK | CLKSEL_MASK;

    constexpr auto ReplyBefore = FIRST_PRI.underlying() + 1;
    constexpr auto ReplyAfter = FIRST_PRI.underlying() - 1;

    testThing<4,ReplyBefore>();
    testThing<4,ReplyAfter>();
    testThing<64,ReplyBefore>();
    testThing<64,ReplyAfter>();
}
}
#endif
