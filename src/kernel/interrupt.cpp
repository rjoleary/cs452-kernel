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

namespace interrupt {
void init() {
    *(volatile unsigned*)(0x38) = (unsigned)irqEntry;
    *(volatile unsigned*)(VIC1Base + VICxDefVectAddr) = 0xdeadbeef;
    *(volatile unsigned*)(VIC2Base + VICxDefVectAddr) = 0xdeadbeef;
}

void clearAll() {
    // Disable all interrupts
    *(volatile unsigned*)(VIC1Base + VICxIntEnClear) = 0xffffffff;
    *(volatile unsigned*)(VIC2Base + VICxIntEnClear) = 0xffffffff;
    // Set all interrupts to IRQ
    *(volatile unsigned*)(VIC1Base + VICxIntSelect) = 0;
    *(volatile unsigned*)(VIC2Base + VICxIntSelect) = 0;
}

void bind(ctl::Source src, unsigned vector) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    // 1. Set the address of the interrupt handler.
    *(volatile void**)(base + VICxVectAddr0 + vector) = nullptr;
    // 2. Set the interrupt source and enable.
    *(volatile unsigned*)(base + VICxVectCntl0 + vector) = 0x20 | iSrc;
    // 3. Enable the interrupt
    *(volatile unsigned*)(base + VICxIntEnable) = 1 << iSrc;
}

int setVal(ctl::Source src, unsigned vector, void *isr) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector);
    if (*addr != nullptr) return -1;
    *addr = isr;
    return 0;
}

void clear(ctl::Source src, unsigned vector) {
    auto iSrc = static_cast<unsigned>(src);
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector);
    *addr = nullptr;
}
}
}
