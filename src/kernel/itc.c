#include <bwio.h>
#include <itc.h>

int kSend(Tid tid, char *msg, int msglen, char *reply, int rplen) {
    // TODO: implement
    bwprintf(COM2, "SYSCALL: send(tid=%d, msg=0x%x, msglen=%d, reply=%x, rplen=%d",
        tid, msg, msglen, reply, rplen);
    return -1;
}

int kReceive(Tid *tid, char *msg, int msglen) {
    // TODO: implement
    bwprintf(COM2, "SYSCALL: send(tid=%d, msg=0x%x, msglen=%d", tid, msg, msglen);
    return -1;
}

int kReply(Tid tid, char *reply, int rplen) {
    // TODO: implement
    bwprintf(COM2, "SYSCALL: send(tid=%d, reply=%x, rplen=%d", tid, reply, rplen);
    return -1;
}
