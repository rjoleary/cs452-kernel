#include <syscall.h>
#include <task.h>

namespace ctl {
int create(Priority pri, void (*a1)()) {
    int a0 = pri.underlying();
    SYSCALL2R(kernel::Syscall::Create)
    return ret;
}
Tid myTid() {
    SYSCALL0R(kernel::Syscall::MyTid)
    return Tid(ret);
}
Tid myParentTid() {
    SYSCALL0R(kernel::Syscall::MyParentTid)
    return Tid(ret);
}
int taskInfo(Tid tid, TaskInfo *a1) {
    unsigned a0 = tid.underlying();
    SYSCALL2R(kernel::Syscall::TaskInfo)
    return ret;
}
void pass() {
    SYSCALL0(kernel::Syscall::Pass)
}
void exeunt() {
    SYSCALL0(kernel::Syscall::Exeunt)
}
void destroy() {
    SYSCALL0(kernel::Syscall::Destroy)
}
}
