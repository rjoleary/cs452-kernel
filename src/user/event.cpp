#include <event.h>
#include <syscall.h>

namespace ctl {
int awaitEvent(InterruptSource a0) {
    SYSCALL0R(kernel::Syscall::AwaitEvent)
    return ret;
}
}
