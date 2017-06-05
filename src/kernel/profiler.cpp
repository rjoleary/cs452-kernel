#include <interrupt.h>
#include <profiler.h>
#include <user/bwio.h>
#include <user/event.h>
#include <user/std.h>
#include <user/ts7200.h>
#include <panic.h>

using namespace kernel;
using namespace ctl;

// Save space when profiles are not enabled.
#ifdef PROF_INTERVAL
constexpr unsigned BUFFER_LEN = 16 * 1024;
#else
constexpr unsigned BUFFER_LEN = 0;
#endif // PROF_INTERVAL

static unsigned buffer[BUFFER_LEN];

// The lower threshold for reporting addresses
constexpr unsigned PROF_THRESH = 3;

extern "C" char textStart, textEnd;

extern "C" void profilerFiq();

extern "C" void profilerRecord(unsigned pc) {
    buffer[(pc - (unsigned)&textStart) / 4]++;
}

void profilerStart(unsigned ticks) {
    unsigned textLen = ((unsigned)&textEnd - (unsigned)&textStart) / 4;
    if (BUFFER_LEN < textLen) {
        bwprintf(COM2, "Error: buffer is not large enough for profiling, must be at least 0x%08x", textLen);
        return;
    }

    // Disable timer 3 interrupt.
    const unsigned iSrc = static_cast<unsigned>(InterruptSource::TC3UI) - 32;
    *(volatile unsigned*)(VIC2Base + VICxIntEnClear) = 1 << iSrc;

    // Setup TIMER3:
    // - 32-bit precision
    // - 508 kHz clock
    // - each 1ms is 508 ticks
    // - periodic mode
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER3_BASE + LDR_OFFSET) = ticks;
    *(volatile unsigned*)(TIMER3_BASE + CRTL_OFFSET) = CLKSEL_MASK | ENABLE_MASK | MODE_MASK;

    // Profiler uses FIQ so that it can even interrupts the kernel.
    *(volatile unsigned*)(0x3c) = (unsigned)profilerFiq;
    *(volatile unsigned*)(VIC2Base + VICxIntSelect) |= 1 << iSrc;
    *(volatile unsigned*)(VIC2Base + VICxIntEnable) |= 1 << iSrc;
}

void profilerStop() {
    const unsigned iSrc = static_cast<unsigned>(InterruptSource::TC3UI) - 32;
    *(volatile unsigned*)(VIC2Base + VICxIntEnClear) = 1 << iSrc;
}

void profilerDump() {
    for (unsigned i = 0; i < BUFFER_LEN; i++) {
        if (buffer[i] >= PROF_THRESH) {
            bwprintf(COM2, "0x%08x: %d\r\n", (unsigned)&textStart + i * 4, buffer[i]);
        }
    }
}
