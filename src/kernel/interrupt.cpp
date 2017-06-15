#include <interrupt.h>
#include <user/bwio.h>
#include <panic.h>
#include <task.h>

extern "C" void irqEntry();

namespace kernel {

namespace interrupt {
void init() {
    clearAll();
    *(volatile unsigned*)(0x38) = (unsigned)irqEntry;
    *(volatile unsigned*)(VIC1Base + VICxIntEnable) =
        (1u << TC1UI);
    *(volatile unsigned*)(VIC2Base + VICxIntEnable) =
    //    (1u << INT_UART1) +
        (1u << INT_UART2);
}

void clearAll() {
    // Disable all interrupts
    *(volatile unsigned*)(VIC1Base + VICxIntEnClear) = 0xffffffff;
    *(volatile unsigned*)(VIC2Base + VICxIntEnClear) = 0xffffffff;
    // Set all interrupts to IRQ
    *(volatile unsigned*)(VIC1Base + VICxIntSelect) = 0;
    *(volatile unsigned*)(VIC2Base + VICxIntSelect) = 0;
}

} // namespace interrupt
} // namespace kernel
