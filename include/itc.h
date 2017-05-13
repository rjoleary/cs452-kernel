// Inter-task communication

#ifndef ITC_H__INCLUDED
#define ITC_H__INCLUDED

#include "types.h"

int kSend(Tid tid, char *msg, int msglen, char *reply, int rplen);
int kReceive(Tid *tid, char *msg, int msglen);
int kReply(Tid tid, char *reply, int rplen);

#endif // ITC_H__INCLUDED
