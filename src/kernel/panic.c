#include <bwio.h>
#include <panic.h>

void panic(unsigned *regs, const char *str, const char *file, int line) {
    // TODO: CPSR and SPSR

    // TODO: disable interrupts
    bwputstr(COM2, "\r\n!!!!!!!! PANIC !!!!!!!!\r\n");
    bwprintf(COM2, "Error: %s\r\n", str);
    bwprintf(COM2, "Location: %s:%d\r\n", file, line);
    bwprintf(COM2, "r0: 0x%08x  r5: 0x%08x  r10: 0x%08x    pc: 0x%08x\r\n",
        regs[0], regs[5], regs[10], regs[15]);
    bwprintf(COM2, "r1: 0x%08x  r6: 0x%08x  r11: 0x%08x\r\n",
        regs[1], regs[6], regs[11]);
    bwprintf(COM2, "r2: 0x%08x  r7: 0x%08x  r12: 0x%08x\r\n",
        regs[2], regs[7], regs[12]);
    bwprintf(COM2, "r3: 0x%08x  r8: 0x%08x   sp: 0x%08x\r\n",
        regs[3], regs[8], regs[13]);
    bwprintf(COM2, "r4: 0x%08x  r9: 0x%08x   lr: 0x%08x\r\n",
        regs[4], regs[9], regs[14]);

    bwputstr(COM2, "\r\nSystem requires reboot\r\n");
    while (1);
}
