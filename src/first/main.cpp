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

template <size_t I, int P, bool silent>
void testThing() {
    constexpr auto AmountToSend = 10'000;//, ClockTicks = 508'000, Microseconds = 1'000'000;
    auto timerVal = (volatile unsigned*)(TIMER3_BASE + VAL_OFFSET);
    auto start = *timerVal;
    if (!silent)
        bwprintf(COM2, "Size %d Pri %d...\r\n", I, P);
    Tid tid = create(Priority(P), perfMain<I>);

    Message<I> msg;
    for (unsigned i = 0; i < AmountToSend; i++) {
        send(tid, msg, msg);
    }

    if (!silent) {
        unsigned elapsed = start - *timerVal; 
        bwprintf(COM2, "Done! Elapsed: %u\r\n", elapsed);
    }
}

void firstMain() {
    bwputstr(COM2, "Running performance test...\r\n");
    constexpr auto ReplyBefore = FIRST_PRI.underlying() + 1;
    constexpr auto ReplyAfter = FIRST_PRI.underlying() - 1;
    testThing<4,ReplyBefore, true>();
    testThing<4,ReplyAfter, true>();
    testThing<64,ReplyBefore, true>();
    testThing<64,ReplyAfter, true>();

    testThing<4,ReplyBefore,false>();
    testThing<4,ReplyAfter,false>();
    testThing<64,ReplyBefore,false>();
    testThing<64,ReplyAfter,false>();
}
}
#endif
