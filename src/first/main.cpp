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

namespace ctl {

struct Message {
    char data[4];
};

void perfMain() {
    while (1) {
        Tid tid;
        Message msg;
        receive(&tid, msg);
        reply(tid, nullptr, 0);
    }
}

void firstMain() {
    bwputstr(COM2, "Running performance test...\r\n");
    Tid tid = create(Priority(1), perfMain);

    // Setup TIMER1:
    // - 32-bit precision
    // - 508 kHz clock
    // - each 1ms is 508 ticks
    // - timer starts at 0xffffffff and counts down
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER3_BASE + LDR_OFFSET) = 0xffffffffU;
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = ENABLE_MASK | CLKSEL_MASK;

    Message msg;
    for (unsigned i = 0; i < 10000; i++) {
        send(tid, &msg, sizeof(msg), nullptr, 0);
    }

    unsigned elapsed = 0xffffffffu - *(volatile unsigned*)(TIMER3_BASE + VAL_OFFSET);
    bwprintf(COM2, "Done! Elapsed: %u\r\n", elapsed);
}

}

#endif
