#include <user/task.h>
#include <user/syscall.h>

Tid create(Priority priority, void (*code)()) SYSCALLR(SYS_CREATE)
Tid myTid(void) SYSCALLR(SYS_MYTID)
Tid myParentTid(void) SYSCALLR(SYS_MYPARENTID)
void pass(void) SYSCALL(SYS_PASS)
void exit_(void) SYSCALL(SYS_EXIT)
