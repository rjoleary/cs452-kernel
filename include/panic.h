#ifndef PANIC_H__INCLUDED
#define PANIC_H__INCLUDED

#define PANIC(str)                        \
    do {                                  \
    asm volatile (                        \
        "stmfd sp!, {r0-r15}"             \
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
