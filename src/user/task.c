#include <task.h>
#include <syscall.h>

Tid create(Priority priority, void (*code)(void)) SYSCALLR(SYS_CREATE)
Tid myTid(void) SYSCALLR(SYS_MYTID)
Tid myParentTid(void) SYSCALLR(SYS_MYPARENTTID)
void pass(void) SYSCALL(SYS_PASS)
void exeunt(void) SYSCALL(SYS_EXEUNT)
void destroy(void) SYSCALL(SYS_DESTROY)
