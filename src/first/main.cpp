#include <bwio.h>
#include <clock.h>
#include <def.h>
#include <event.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <itc.h>

#ifndef PERF_TEST

// Forward declaration.
void idleMain();

namespace ctl {
void nsMain();
void clockNotifier();
void clockMain();

namespace {
struct Message {
    Priority p; // priority
    int t; // delay time
    int n; // number of delays
};

void clientMain() {
    Message rply;
    ASSERT(send(myParentTid(), EmptyMessage, rply) == sizeof(rply));
    auto clockTid = Tid(whoIs(Names::ClockServer));
    for (int i = 0; i < rply.n; i++) {
        delay(clockTid, rply.t);
        bwprintf(COM2, "Tid: %d, MyDelay: %d, TotalDelay: %d\r\n",
            myTid(), rply.t, time(clockTid));
    }

    // Send exit message to parent. Does not return
    send(myParentTid(), EmptyMessage, EmptyMessage);
}
}

void firstMain() {
    ASSERT(Tid(create(PRIORITY_MIN, idleMain)) == IDLE_TID);
    ASSERT(Tid(create(PRIORITY_MAX, nsMain)) == NS_TID);
    ASSERT(create(Priority(30), clockMain) >= 0);
    ASSERT(create(Priority(30), clockNotifier) >= 0);

    Message msgs[] = {
        {Priority{6}, 10, 20},
        {Priority{5}, 23, 9},
        {Priority{4}, 33, 6},
        {Priority{3}, 71, 3},
    };

    // Create tasks.
    for (const auto &msg : msgs) {
        auto ret = create(msg.p, clientMain);
        ASSERT(ret > 0);
    }

    // Send parameters.
    for (const auto &msg : msgs) {
        Tid tid;
        ASSERT(receive(&tid, EmptyMessage) == 0);
        ASSERT(reply(tid, msg) == 0);
    }

    // Block until the tasks finish.
    for (const auto &msg : msgs) {
        (void)msg;
        Tid tid;
        ASSERT(receive(&tid, EmptyMessage) == 0);
    }
}
}

#else

#include <ts7200.h>
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
