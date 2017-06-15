#include <event.h>
#include <syscall.h>

namespace ctl {
int awaitEvent(Event a0) {
    SYSCALL1R(kernel::Syscall::AwaitEvent)
    return ret;
}
}
