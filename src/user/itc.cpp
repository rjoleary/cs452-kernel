#include <itc.h>

namespace ctl {
int send(Tid tid, const void *a1, int a2, void *a3, int a4) {
    int a0 = tid.underlying();
    SYSCALL5R(kernel::Syscall::Send)
    return ret;
}
int receive(Tid *a0, void *a1, int a2) {
    SYSCALL3R(kernel::Syscall::Receive)
    return ret;
}
int reply(Tid tid, const void *a1, int a2) {
    int a0 = tid.underlying();
    SYSCALL3R(kernel::Syscall::Reply)
    return ret;
}
}
