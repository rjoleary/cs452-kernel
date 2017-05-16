#include <itc.h>

int send(Tid tid, char *msg, int msglen, char *reply, int rplen) SYSCALLR(SYS_SEND)
int receive(Tid *tid, char *msg, int msglen) SYSCALLR(SYS_RECEIVE)
int reply(Tid tid, char *reply, int rplen) SYSCALLR(SYS_REPLY)
