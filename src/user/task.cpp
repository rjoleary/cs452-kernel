#include <syscall.h>
#include <task.h>

namespace ctl {
Tid create(Priority a0, void (*a1)()) SYSCALL2R(kernel::Syscall::Create)
Tid myTid() SYSCALL0R(kernel::Syscall::MyTid)
Tid myParentTid() SYSCALL0R(kernel::Syscall::MyParentTid)
void pass() SYSCALL0(kernel::Syscall::Pass)
void exeunt() SYSCALL0(kernel::Syscall::Exeunt)
void destroy() SYSCALL0(kernel::Syscall::Destroy)
}
