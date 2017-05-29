#include <bwio.h>
#include <def.h>
#include <task.h>

#ifndef PERF_TEST

// Forward declaration.
void rpsServerMain();
void rpsClientMain();

namespace ctl {
void nsMain();

void firstMain() {
    bwprintf(COM2, "FirstUserTask: myTid()=%d\r\n", myTid());
    bwprintf(COM2, "FirstUserTask: created nameserver, tid %d\r\n",
            create(PRIORITY_MAX, nsMain));

    // Create rock paper scissors server and clients.
    create(Priority(5), rpsServerMain);
    for (unsigned i = 0; i < NUM_RPS_CLIENTS; i++) {
        create(Priority(2), rpsClientMain);
    }
}
}

#else

#include <ts7200.h>
#include <itc.h>
#include <std.h>

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
    constexpr auto AmountToSend = 10'000, ClockTicks = 508'000, Microseconds = 1'000'000;
    auto timerVal = (volatile unsigned*)(TIMER3_BASE + VAL_OFFSET);
    auto start = *timerVal;
    bwprintf(COM2, "Size %d Pri %d...\r\n", I, P);
    Tid tid = create(Priority(P), perfMain<I>);

    Message<I> msg;
    for (unsigned i = 0; i < AmountToSend; i++) {
        send(tid, msg, msg);
    }

    unsigned elapsed = start - *timerVal; 
    bwprintf(COM2, "Done! Elapsed: %u\r\n", elapsed);
    unsigned averageTrunc = elapsed/double(ClockTicks)*Microseconds/AmountToSend;
    bwprintf(COM2, "Average time: %u\r\n", averageTrunc);
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
    testThing<4,31>();
    testThing<4,1>();
    testThing<64,31>();
    testThing<64,1>();
}
}
#endif
