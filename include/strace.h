#ifndef STRACE_H__INCLUDED
#define STRACE_H__INCLUDED

#include <user/bwio.h>

// Colors for the kernel.
#define BEGIN_SYS_CL "\033[30;1m"
#define END_CL "\033[0m"

// Enable syscall trace using:
//
//   make STRACE_ENABLED=1
//
#ifdef STRACE_ENABLED
#define STRACE(fmt, ...) do {                                          \
        bwprintf(COM2, BEGIN_SYS_CL fmt END_CL "\r\n", ##__VA_ARGS__); \
    } while (0)
#else
#define STRACE(fmt, ...) (void)0
#endif


#endif // STRACE_H__INCLUDED
