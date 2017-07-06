#pragma once

namespace kernel {
enum class Syscall {
    Create = 0,
    MyTid,
    MyParentTid,
    TaskInfo,
    Pass,
    Exeunt,
    Destroy,
    Send,
    Receive,
    Reply,
    AwaitEvent,
};
}

// Clang is only used for autocomplete and testing. The toolchain for
#ifdef __clang__
#define SYSCALL0(id) (void)id;
#define SYSCALL1(id) (void)id; (void)a0;
#define SYSCALL0R(id) int ret = 0; (void)id;
#define SYSCALL1R(id) int ret = 0; (void)id; (void)a0;
#define SYSCALL2R(id) int ret = 0; (void)id; (void)a0; (void)a1;
#define SYSCALL3R(id) int ret = 0; (void)id; (void)a0; (void)a1; (void)a2;
#define SYSCALL5R(id) int ret = 0; (void)id; (void)a0; (void)a1; (void)a2; (void)a3; (void)a4;
#else

// Body for a syscall taking 0 arguments.
#define SYSCALL0(id)            \
    asm volatile (              \
        "mov r7, %0\n\t"        \
        "swi #0"                \
        :                       \
        : "i" (id)              \
        : "r7"                  \
        , "memory"              \
    );

// Body for a syscall taking 1 argument.
#define SYSCALL1(id)                \
    register int r0 asm("r0") = a0; \
    asm volatile (                  \
        "mov r7, %0\n\t"            \
        "swi #0"                    \
        :                           \
        : "i" (id)                  \
        , "r" (r0)                  \
        : "r7"                      \
        , "memory"                  \
    );

// Body for a syscall taking 0 arguments and returning an int.
#define SYSCALL0R(id)           \
    register int ret asm("r0"); \
    asm volatile (              \
        "mov r7, %1\n\t"        \
        "swi #0"                \
        : "=r" (ret)            \
        : "i" (id)              \
        : "r7"                  \
        , "memory"              \
    );

// Body for a syscall taking 1 argument and returning an int.
#define SYSCALL1R(id)                              \
    register int ret asm("r0");                    \
    register unsigned r0 asm("r0") = (unsigned)a0; \
    asm volatile (                                 \
        "mov r7, %1\n\t"                           \
        "swi #0"                                   \
        : "=r" (ret)                               \
        : "i" (id)                                 \
        , "r" (r0)                                 \
        : "r7"                                     \
        , "memory"                                 \
    );

// Body for a syscall taking 2 arguments and returning an int.
#define SYSCALL2R(id)                              \
    register int ret asm("r0");                    \
    register unsigned r0 asm("r0") = (unsigned)a0; \
    register unsigned r1 asm("r1") = (unsigned)a1; \
    asm volatile (                                 \
        "mov r7, %1\n\t"                           \
        "swi #0"                                   \
        : "=r" (ret)                               \
        : "i" (id)                                 \
        , "r" (r0)                                 \
        , "r" (r1)                                 \
        : "r7"                                     \
        , "memory"                                 \
    );

// Body for a syscall taking 3 arguments and retuning an int.
#define SYSCALL3R(id)                              \
    register int ret asm("r0");                    \
    register unsigned r0 asm("r0") = (unsigned)a0; \
    register unsigned r1 asm("r1") = (unsigned)a1; \
    register unsigned r2 asm("r2") = (unsigned)a2; \
    asm volatile (                                 \
        "mov r7, %1\n\t"                           \
        "swi #0"                                   \
        : "=r" (ret)                               \
        : "i" (id)                                 \
        , "r" (r0)                                 \
        , "r" (r1)                                 \
        , "r" (r2)                                 \
        : "r7"                                     \
        , "memory"                                 \
    );

// Body for a syscall taking 5 arguments and retuning an int.
#define SYSCALL5R(id)                              \
    register int ret asm("r0");                    \
    register unsigned r0 asm("r0") = (unsigned)a0; \
    register unsigned r1 asm("r1") = (unsigned)a1; \
    register unsigned r2 asm("r2") = (unsigned)a2; \
    register unsigned r3 asm("r3") = (unsigned)a3; \
    register unsigned r4 asm("r4") = (unsigned)a4; \
    asm volatile (                                 \
        "mov r7, %1\n\t"                           \
        "swi #0"                                   \
        : "=r" (ret)                               \
        : "i" (id)                                 \
        , "r" (r0)                                 \
        , "r" (r1)                                 \
        , "r" (r2)                                 \
        , "r" (r3)                                 \
        , "r" (r4)                                 \
        : "r7"                                     \
        , "memory"                                 \
    );

#endif // __clang__
