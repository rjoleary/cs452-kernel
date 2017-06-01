#include <interrupt.h>

namespace {
// Manual for the interrupt controller:
//     http://infocenter.arm.com/help/topic/com.arm.doc.ddi0181e/DDI0181.pdf
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

volatile char * deduceDaisyChain(unsigned &iSrc) {
    auto base = iSrc < 32 ? VIC1Base : VIC2Base;
    iSrc %= 32;
    return base;
}
}

extern "C" void irqEntry();

namespace kernel {

void initInterrupts() {
    // irqTrampoline reads vector address into pc.
    *(volatile unsigned*)(0x38) = (unsigned)irqEntry;
    *(volatile unsigned*)(VIC1Base + VICxDefVectAddr) = 0xdeadbeef;
    *(volatile unsigned*)(VIC2Base + VICxDefVectAddr) = 0xdeadbeef;
}

void bindInterrupt(ctl::InterruptSource src, unsigned vector, void *isr) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    // 1. Clear any existing interrupt.
    *(volatile unsigned*)(base + VICxIntEnClear) = 1 << iSrc;
    // 2. Set the address of the interrupt handler.
    *(volatile unsigned*)(base + VICxVectAddr0 + vector) = (unsigned)isr;
    // 3. Set the interrupt source and enable.
    *(volatile unsigned*)(base + VICxVectCntl0 + vector) = 0x20 | iSrc;
    // 4. Ensure the interrupt type is set to IRQ.
    *(volatile unsigned*)(base + VICxIntSelect) &= ~(1 << iSrc);
}

int enableOnly(ctl::InterruptSource src, unsigned vector, void *isr) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector);
    if (*addr != nullptr) return -1;
    *addr = isr;
    return 0;
}

void clearInterrupt(ctl::InterruptSource src, unsigned vector) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector);
    *addr = nullptr;
}

void enableInterrupt(ctl::InterruptSource src) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    *(volatile unsigned*)(base + VICxIntEnable) |= 1 << iSrc;
}

void disableInterrupt(ctl::InterruptSource src) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    *(volatile unsigned*)(base + VICxIntEnClear) = 1 << iSrc;
}
}
