// Calling PANIC(str) will halt the system and dump all the registers and stack
// to COM2. The first argument, str, can be an informational message.
#pragma once

#define PANIC(str)                                        \
    do {                                                  \
    asm volatile (                                        \
        "stmfd sp, {r0-r15}\n\t"                          \
        "add sp, sp, #-64\n\t"                            \
        "mrs r0, CPSR\n\t"                                \
        "mrs r1, SPSR\n\t"                                \
        "stmfd sp, {r0,r1}\n\t"                           \
        "msr CPSR, #0xd3\n\t"                             \
        "add sp, sp, #-8"                                 \
    );                                                    \
    unsigned *regs;                                       \
    asm volatile (                                        \
        "mov %0, sp"                                      \
        : "=r" (regs)                                     \
    );                                                    \
    kernel::detail::panic(regs, str, __FILE__, __LINE__); \
    __builtin_unreachable();                              \
    } while(0)

namespace kernel {
namespace detail {
// This function is used internally by the PANIC macro. The reason a macro is
// needed at all is because function calls touch the registers.
void panic(unsigned *regs, const char *str, const char *file, int line);
}
}
