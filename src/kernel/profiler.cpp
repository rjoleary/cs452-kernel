#include <profiler.h>
#include <user/bwio.h>
#include <user/ts7200.h>
#include <user/std.h>

volatile char *const VIC1Base = (char *)(0x800b0000);
volatile char *const VIC2Base = (char *)(0x800c0000);
const unsigned
    VICxIRQStatus    = 0x00,
    VICxFIQStatus    = 0x04,
    VICxRawIntr      = 0x08,
    VICxIntSelect    = 0x0c,
    VICxIntEnable    = 0x10,
    VICxIntEnClear   = 0x14,
    VICxSoftInt      = 0x18,
    VICxSoftIntClear = 0x1c,
    VICxProtection   = 0x20,
    VICxVectAddr     = 0x30,
    VICxDefVectAddr  = 0x34,
    VICxVectAddr0    = 0x100,
    VICxVectCntl0    = 0x200;

// TODO: optional
constexpr unsigned BUFFER_LEN = 16*1024;
static unsigned buffer[BUFFER_LEN];

// The lower threshold for reporting addresses
constexpr unsigned PROF_THRESH = 3;

extern "C" char textStart, textEnd;

extern "C" void timer2Irq();
extern "C" void irqTrampoline();

extern "C" void profilerRecord(unsigned lr) {
    // Minux 4 because of the interrupt.
    buffer[(lr - (unsigned)&textStart - 4) / 4]++;
}

void profilerStart(unsigned ticks) {
    unsigned textLen = ((unsigned)&textEnd - (unsigned)&textStart) / 4;
    if (BUFFER_LEN < textLen) {
        bwprintf(COM2, "Error: buffer is not large enough for profiling, must be at least 0x%08x", textLen);
        return;
    }

    memset(buffer, 0, sizeof(buffer));

    // Setup TIMER2:
    // - 16-bit precision
    // - 508 kHz clock
    // - each 1ms is 508 ticks
    // - periodic mode
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = 0;
    *(volatile unsigned*)(TIMER2_BASE + LDR_OFFSET) = ticks;
    *(volatile unsigned*)(TIMER2_BASE + CRTL_OFFSET) = CLKSEL_MASK | ENABLE_MASK | MODE_MASK;

    // FIQ reads from vector address.
    //*((volatile unsigned*)(0x18)) = fiqVector;
    //*((volatile unsigned*)(0x38)) = (unsigned)(VIC1Base + VICxVectAddr);
    *((volatile unsigned*)(0x38)) = (unsigned)irqTrampoline;
    *((volatile unsigned*)(VIC1Base + VICxDefVectAddr)) = 0xdeadbeef;

    // Timer 2 is interrupt source 5 on the first VIC with vectored interrupts.
    // Manual for the interrupt controller:
    //     http://infocenter.arm.com/help/topic/com.arm.doc.ddi0181e/DDI0181.pdf
    // 1. Clear any existing interrupt.
    *((volatile unsigned*)(VIC1Base + VICxIntEnClear)) = 1 << 5;
    // 2. Set the address of the interrupt handler.
    *((volatile unsigned*)(VIC1Base + VICxVectAddr0)) = (unsigned)timer2Irq;
    // 3. Set the interrupt source and enable.
    *((volatile unsigned*)(VIC1Base + VICxVectCntl0)) = 0x20 | 5;
    // 4. Ensure the interrupt type is set to IRQ.
    *((volatile unsigned*)(VIC1Base + VICxIntSelect)) &= ~(1 << 5);
    // 5. Enable the interrupt.
    *((volatile unsigned*)(VIC1Base + VICxIntEnable)) |= 1 << 5;
}

void profilerStop() {
    // Disable the interrupt.
    *((volatile unsigned*)(VIC1Base + VICxIntEnClear)) = 1 << 5;
}

void profilerDump() {
    for (unsigned i = 0; i < BUFFER_LEN; i++) {
        if (buffer[i] >= PROF_THRESH) {
            bwprintf(COM2, "0x%08x: %d\r\n", (unsigned)&textStart + i * 4, buffer[i]);
        }
    }
}
