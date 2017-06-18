// Inter-task communication
#pragma once

#include "syscall.h"
#include "types.h"
#include "err.h"
#include <type_traits>

namespace ctl {

struct EmptyMessage_t{};
extern EmptyMessage_t EmptyMessage;
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

namespace detail {
inline int send (int a0, const void *a1, int a2, void *a3, int a4) {
    SYSCALL5R(kernel::Syscall::Send);
    return ret;
}
}
template <typename T, typename U>
ErrorOr<void> send(Tid tid, const T &msg, const U &reply) = delete;
template <typename T, typename U>
ErrorOr<void> send(Tid tid, const T &msg, U &reply) {
    constexpr bool isEmptyMsg = std::is_same<std::decay_t<T>, EmptyMessage_t>::value;
    constexpr bool isEmptyRpl = std::is_same<std::decay_t<U>, EmptyMessage_t>::value;
    static_assert(alignof(T) >= alignof(unsigned) || isEmptyMsg, "Unaligned send msg");
    static_assert(alignof(U) >= alignof(unsigned) || isEmptyRpl, "Unaligned send reply");
    static_assert((sizeof(T) % sizeof(unsigned)) == 0 || isEmptyMsg, "Bad send msg size");
    static_assert((sizeof(U) % sizeof(unsigned)) == 0 || isEmptyRpl, "Bad send reply size");
    int a0 = tid.underlying();
    const void *a1 = isEmptyMsg ? nullptr : &msg;
    int a2 = isEmptyMsg ? 0 : sizeof(T);
    void *a3 = isEmptyRpl ? nullptr : &reply;
    int a4 = isEmptyRpl ? 0 : sizeof(U);
    int err = detail::send(a0, a1, a2, a3, a4);
    if (isEmptyRpl && err > 0) {
        return ErrorOr<void>::fromError(Error::BadItc);
    }
    if (err >= 0 && err != sizeof(reply)) {
        return ErrorOr<void>::fromError(Error::BadItc);
    }
    return ErrorOr<void>::fromError(static_cast<Error>(err));
}

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
//   -ERR_BADITC

namespace detail {
inline int receive(Tid *a0, void *a1, int a2) {
    SYSCALL3R(kernel::Syscall::Receive);
    return ret;
}
}
template <typename T>
ErrorOr<void> receive(Tid *a0, T &msg) {
    constexpr bool isEmpty = std::is_same<std::decay_t<T>, EmptyMessage_t>::value;
    static_assert(alignof(T) >= alignof(unsigned) || isEmpty, "Unaligned receive");
    static_assert((sizeof(T) % sizeof(unsigned)) == 0 || isEmpty, "Bad receive size");
    void *a1 = isEmpty ? nullptr : &msg;
    int a2 = isEmpty ? 0 : sizeof(T);
    int err = detail::receive(a0, a1, a2);
    if (isEmpty && err > 0) {
        return ErrorOr<void>::fromError(Error::BadItc);
    }
    if (err >= 0 && err != sizeof(msg)) {
        return ErrorOr<void>::fromError(Error::BadItc);
    }
    return ErrorOr<void>::fromError(static_cast<Error>(err));
}

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
namespace detail {
inline int reply(int a0, const void *a1, int a2) {
    SYSCALL3R(kernel::Syscall::Reply)
    return ret;
}
}

template <typename T>
ErrorOr<void> reply(Tid tid, const T &msg) {
    constexpr bool isEmpty = std::is_same<std::decay_t<T>, EmptyMessage_t>::value;
    static_assert(alignof(T) >= alignof(unsigned) || isEmpty, "Unaligned reply");
    static_assert((sizeof(T) % sizeof(unsigned)) == 0 || isEmpty, "Bad reply size");
    int a0 = tid.underlying();
    const void *a1 = isEmpty ? nullptr : &msg;
    int a2 = isEmpty ? 0 : sizeof(T);
    return ErrorOr<void>::fromError(static_cast<Error>(-detail::reply(a0, a1, a2)));
}
}
