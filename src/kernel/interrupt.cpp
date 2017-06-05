#include <interrupt.h>

extern "C" void irqEntry();

namespace kernel {
namespace {
volatile char * deduceDaisyChain(unsigned &iSrc) {
    auto base = iSrc < 32 ? VIC1Base : VIC2Base;
    iSrc %= 32;
    return base;
}
}

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
