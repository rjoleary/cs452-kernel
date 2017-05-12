#include <bwio.h>
#include <panic.h>

void panic(const char *str, const char *file, int line) {
    // TODO: these are the registers after calling the function.
    register unsigned *r0 asm("r0");
    register unsigned *r1 asm("r1");
    register unsigned *r2 asm("r2");
    register unsigned *r3 asm("r3");
    register unsigned *r4 asm("r4");
    register unsigned *r5 asm("r5");
    register unsigned *r6 asm("r6");
    register unsigned *r7 asm("r7");
    register unsigned *r8 asm("r8");
    register unsigned *r9 asm("r9");
    register unsigned *r10 asm("r10");
    register unsigned *r11 asm("r11");
    register unsigned *r12 asm("r12");
    register unsigned *sp asm("sp");
    register unsigned *lr asm("lr");
    register unsigned *pc asm("pc");
    // TODO: CPSR and SPSR

    // TODO: disable interrupts
    bwputstr(COM2, "\r\n!!!!!!!! PANIC !!!!!!!!\r\n");
    bwprintf(COM2, "Error: %s\r\n", str);
    bwprintf(COM2, "Location: %s:%d\r\n", file, line);
    bwprintf(COM2, "r0: 0x%08x  r5: 0x%08x  r10: 0x%08x    pc: 0x%08x\r\n", r0, r5, r10, pc);
    bwprintf(COM2, "r1: 0x%08x  r6: 0x%08x  r11: 0x%08x\r\n", r1, r6, r11);
    bwprintf(COM2, "r2: 0x%08x  r7: 0x%08x  r12: 0x%08x\r\n", r2, r7, r12);
    bwprintf(COM2, "r3: 0x%08x  r8: 0x%08x   sp: 0x%08x\r\n", r3, r8, sp);
    bwprintf(COM2, "r4: 0x%08x  r9: 0x%08x   lr: 0x%08x\r\n", r4, r9, lr);

    // TODO: restart machine
    while (1);
}
