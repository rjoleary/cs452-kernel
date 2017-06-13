#include <interrupt.h>
#include <user/bwio.h>

extern "C" void irqEntry();

namespace kernel {
namespace {
volatile char * deduceDaisyChain(unsigned &iSrc) {
    auto base = iSrc < 32 ? VIC1Base : VIC2Base;
    iSrc %= 32;
    return base;
}

// Maps from interrupt source to interrupt vector.
// TODO: remove global
static int src2vec[64];
}

namespace interrupt {
void init() {
    src2vec[(int)ctl::Source::TC1UI] = 0;
    src2vec[(int)ctl::Source::TC2UI] = 1;
    src2vec[(int)ctl::Source::INT_UART1] = 2;
    src2vec[(int)ctl::Source::INT_UART2] = 3;

    clearAll();
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

void bind(ctl::Source src) {
    auto iSrc = static_cast<unsigned>(src);
    auto vector = src2vec[iSrc];
    auto base = deduceDaisyChain(iSrc);
    // 1. Set the address of the interrupt handler.
    *(volatile void**)(base + VICxVectAddr0 + vector*4) = nullptr;
    // 2. Set the interrupt source and enable.
    *(volatile unsigned*)(base + VICxVectCntl0 + vector*4) = 0x20 | iSrc;
    // 3. Enable the interrupt
    *(volatile unsigned*)(base + VICxIntEnable) = 1 << iSrc;
}

int setVal(ctl::Source src, void *isr) {
    auto iSrc = static_cast<unsigned>(src);
    auto vector = src2vec[iSrc];
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector*4);
    if (*addr != nullptr) return -1;
    *addr = isr;
    return 0;
}

void clear(ctl::Source src) {
    auto iSrc = static_cast<unsigned>(src);
    auto vector = src2vec[iSrc];
    auto base = deduceDaisyChain(iSrc);
    auto addr = (volatile void**)(base + VICxVectAddr0 + vector*4);
    *addr = nullptr;
}
}
}
