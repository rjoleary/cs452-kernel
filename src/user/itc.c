#include <itc.h>

int send(Tid a0, char *a1, int a2, char *a3, int a4) SYSCALL5R(SYS_SEND)
int receive(Tid *a0, char *a1, int a2) SYSCALL3R(SYS_RECEIVE)
int reply(Tid a0, char *a1, int a2) SYSCALL3R(SYS_REPLY)
