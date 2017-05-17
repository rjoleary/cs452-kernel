#include <task.h>
#include <syscall.h>

Tid create(Priority a0, void (*a1)(void)) SYSCALL2R(SYS_CREATE)
Tid myTid(void) SYSCALL0R(SYS_MYTID)
Tid myParentTid(void) SYSCALL0R(SYS_MYPARENTTID)
void pass(void) SYSCALL0(SYS_PASS)
void exeunt(void) SYSCALL0(SYS_EXEUNT)
void destroy(void) SYSCALL0(SYS_DESTROY)
