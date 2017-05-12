// Inter-task communication

#ifndef USER_ITC_H__INCLUDED
#define USER_ITC_H__INCLUDED

#include "types.h"

// send - send a message to a specific task and obtain the corresponding response.
//
// Description:
//   send sends a message to another task and receives a reply. The message, in
//   a buffer in the sending task's address space is copied to the address
//   space of the task to which it is sent by the kernel. Send supplies a
//   buffer into which the reply is to be copied, and the size of the buffer so
//   that the kernel can detect overflow. When send returns without error it is
//   guaranteed that the message has been received, and that a reply has been
//   sent, not necessarily by the same task. If either the message or the reply
//   is a string it is necessary that the length includes that terminating
//   null.
//
//   The kernel will not overflow the reply buffer. The caller is expected to
//   compare the return value to the size of the reply buffer. If part of the
//   reply is missing the return value will exceed the size of the supplied
//   reply buffer.
//
//   There is no guarantee that send will return. If, for example, the task to
//   which the message is directed never calls receive, send never returns and
//   the sending task remains blocked forever.
//
//   send has a passing resemblance, and no more, to remote procedure call.
//
// Returns:
//   >-1: The size of the message responded by the replying task. The message
//        is less than or equal to the size of the buffer provided for it.
//        Longer responses are truncated.
//   -ERR_TRUNC: The reply message was truncated.
//   -ERR_INVID: The task id supplied is not the task id of an existing task.
//   -ERR_BADITC: The send-receive-reply transaction could not be completed.
int send(Tid tid, char *msg, int msglen, char *reply, int rplen);

// receive - receive a message from a task.
//
// Description:
//   receive blocks until a message is sent to the caller, then returns with
//   the message in its message buffer and tid set to the task id of the task
//   that sent the message. Messages sent before receive is called are retained
//   in a send queue, from which they are received in first-come, first-served
//   order.
//
//   The argument msg must point to a buffer at least as large as msglen. If
//   the size of the message received exceeds msglen, no overflow occurs and
//   the buffer will contain the first msglen characters of the message sent.
//
//   The caller is expected to compare the return value, which contains the
//   size of the message that was sent, to determine whether or not the message
//   is complete, and to act accordingly.
//
// Returns:
//   >-1: The size of the message received, which is less than or equal to the
//        size of the message buffer supplied. Longer messages are truncated.
//   -ERR_TRUNC: The message is truncated.
int receive(Tid *tid, char *msg, int msglen);

// reply - reply to a message.
//
// Description:
//   reply sends a reply to a task that previously sent a message. When it
//   returns without error, the reply has been copied into the sender's address
//   space. The calling task and the sender return at the same logical time, so
//   whichever is of higher priority runs first. If they are of the same
//   priority the sender runs first.
//
// Returns
//   -ERR_OK: The reply succeeded.
//   -ERR_TRUNC: The message was truncated.
//   -ERR_INVID: The task id is not the task id of an existing task.
//   -ERR_BADITC: The task id is not the task id of a reply blocked task.
int reply(Tid tid, char *reply, int rplen);

#endif // USER_ITC_H__INCLUDED
