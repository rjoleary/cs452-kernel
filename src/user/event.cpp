#include <event.h>
#include <syscall.h>

namespace ctl {
int awaitEvent(Source a0) {
    SYSCALL0R(kernel::Syscall::AwaitEvent)
    return ret;
}
}
