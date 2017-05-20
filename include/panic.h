#ifndef PANIC_H__INCLUDED
#define PANIC_H__INCLUDED

#define PANIC(str)                        \
    do {                                  \
    asm volatile (                        \
        "stmfd sp, {r0-r15}\n\t"          \
        "add sp, sp, #-64\n\t"            \
        "mrs r0, CPSR\n\t"                \
        "mrs r1, SPSR\n\t"                \
        "stmfd sp, {r0,r1}\n\t"           \
        "msr CPSR, #0xd3\n\t"             \
        "add sp, sp, #-8"                 \
    );                                    \
    unsigned *regs;                       \
    asm volatile (                        \
        "mov %0, sp"                      \
        : "=r" (regs)                     \
    );                                    \
    panic(regs, str, __FILE__, __LINE__); \
    } while(0)

void panic(unsigned *regs, const char *str, const char *file, int line);

#endif // PANIC_H__INCLUDED
