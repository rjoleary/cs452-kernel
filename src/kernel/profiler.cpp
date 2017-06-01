#include <interrupt.h>
#include <profiler.h>
#include <user/bwio.h>
#include <user/std.h>
#include <user/ts7200.h>

// TODO: make optional to save space
constexpr unsigned BUFFER_LEN = 16 * 1024;
static unsigned buffer[BUFFER_LEN];

// The lower threshold for reporting addresses
constexpr unsigned PROF_THRESH = 3;

extern "C" char textStart, textEnd;

extern "C" void timer2Irq();

extern "C" void profilerRecord(unsigned lr) {
    // Minus 4 because of the interrupt.
    buffer[(lr - (unsigned)&textStart - 4) / 4]++;
}

void profilerStart(unsigned ticks) {
    unsigned textLen = ((unsigned)&textEnd - (unsigned)&textStart) / 4;
    if (BUFFER_LEN < textLen) {
        bwprintf(COM2, "Error: buffer is not large enough for profiling, must be at least 0x%08x", textLen);
        return;
    }

    // Setup TIMER2:
    // - 16-bit precision
    // - 508 kHz clock
    // - each 1ms is 508 ticks
    // - periodic mode
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER2_BASE + LDR_OFFSET) = ticks;
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = CLKSEL_MASK | ENABLE_MASK | MODE_MASK;

    // TODO: Fix
    //kernel::bindInterrupt(ctl::InterruptSource::TC2UI, 1, &timer2Irq);
    kernel::enableInterrupt(ctl::InterruptSource::TC2UI);
}

void profilerStop() {
    kernel::disableInterrupt(ctl::InterruptSource::TC2UI);
}

void profilerDump() {
    for (unsigned i = 0; i < BUFFER_LEN; i++) {
        if (buffer[i] >= PROF_THRESH) {
            bwprintf(COM2, "0x%08x: %d\r\n", (unsigned)&textStart + i * 4, buffer[i]);
        }
    }
}
