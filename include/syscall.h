#ifndef SYSCALL_H__INCLUDED
#define SYSCALL_H__INCLUDED

#include "user/syscall.h"

struct Request;

void kernel_entry(void);
enum Syscall kernel_exit(void **sp, struct Request *);

// Data structure copies function call from user stack to kernel stack.
struct Request {
    // Up to five arguments
    unsigned a[5];
} __attribute__((packed));

#endif // SYCALL_H__INCLUDED
