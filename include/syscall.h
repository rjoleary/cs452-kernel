#ifndef SYSCALL_H__INCLUDED
#define SYSCALL_H__INCLUDED

#include "user/syscall.h"

struct Request;

void kernel_entry(void);
void * kernel_exit(void *sp);

#endif // SYCALL_H__INCLUDED
