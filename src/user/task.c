#include <syscall.h>
#include <task.h>

Tid create(Priority a0, void (*a1)()) SYSCALL2R(SYS_CREATE)
Tid myTid() SYSCALL0R(SYS_MYTID)
Tid myParentTid() SYSCALL0R(SYS_MYPARENTTID)
void pass() SYSCALL0(SYS_PASS)
void exeunt() SYSCALL0(SYS_EXEUNT)
void destroy() SYSCALL0(SYS_DESTROY)
