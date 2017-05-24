#include <itc.h>

namespace ctl {
int send(Tid a0, char *a1, int a2, char *a3, int a4) SYSCALL5R(kernel::Syscall::Send)
int receive(Tid *a0, char *a1, int a2) SYSCALL3R(kernel::Syscall::Receive)
int reply(Tid a0, char *a1, int a2) SYSCALL3R(kernel::Syscall::Reply)
}
