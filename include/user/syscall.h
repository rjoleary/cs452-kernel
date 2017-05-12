#ifndef USER_SYSCALL_H__INCLUDED
#define USER_SYSCALL_H__INCLUDED

enum Syscall {
    SYS_CREATE = 1,
    SYS_MYTID,
    SYS_MYPARENTID,
    SYS_PASS,
    SYS_EXIT,
    SYS_DESTROY,
    SYS_SEND,
    SYS_RECEIVE,
    SYS_REPLY,
    SYS_AWAITEVENT,
};

// Append to syscall with no return value.
#define SYSCALL(id) { \
    asm volatile (    \
        "swi %0"      \
        :             \
        : "i" (id)    \
    );                \
}

// Append to syscall with a return value.
#define SYSCALLR(id) { \
    asm volatile (     \
        "swi %0"       \
        :              \
        : "i" (id)     \
    );                 \
    return -1;         \
}

#endif // USER_SYSCALL_H__INCLUDED
