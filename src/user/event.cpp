#include <event.h>
#include <syscall.h>

namespace ctl {
int awaitEvent(Event a0, int a1) {
    SYSCALL2R(kernel::Syscall::AwaitEvent)
    return ret;
}
}
