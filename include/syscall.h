#ifndef SYSCALL_H__INCLUDED
#define SYSCALL_H__INCLUDED

#include "user/syscall.h"

void kernel_entry(void);
enum Syscall kernel_exit(void **sp);

// Data structure copies function call from user stack to kernel stack.
struct Request {
    enum Syscall syscall;
    // Up to five arguments
    unsigned a[5];
    // One return value
    unsigned r0;
} __attribute__((packed));

#endif // SYCALL_H__INCLUDED
