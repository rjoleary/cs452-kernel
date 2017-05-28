// Inter-task communication

#ifndef USER_ITC_H__INCLUDED
#define USER_ITC_H__INCLUDED

#include "syscall.h"
#include "types.h"

namespace ctl {

struct EmptyMessage_t{};
extern EmptyMessage_t EmptyMessage;
namespace detail {
    template <typename T, typename U>
    struct is_same {
        constexpr static auto value = false;
    };
    template <typename T>
    struct is_same<T, T> {
        constexpr static auto value = true;
    };
    template <typename T, typename U>
    constexpr static auto is_same_v = is_same<T,U>::value;

    template <typename T> struct remove_reference      {using type = T;};
    template <typename T> struct remove_reference<T&>  {using type = T;};
    template <typename T> struct remove_reference<T&&> {using type = T;};

    template <typename T> struct remove_const          {using type = T;};
    template <typename T> struct remove_const<const T> {using type = T;};
     
    template <typename T> struct remove_volatile             {using type = T;};
    template <typename T> struct remove_volatile<volatile T> {using type = T;};

    template <typename T>
    struct remove_cv {
        using type = typename remove_volatile<typename remove_const<T>::type>::type;
    };
     
    template <typename T>
    struct decay {
        using type = typename remove_cv<typename remove_reference<T>::type>::type;
    };
    template <typename T>
    using decay_t = typename decay<T>::type;
}
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
int send(Tid tid, const T &msg, const U &reply) = delete;
template <typename T, typename U>
int send(Tid tid, const T &msg, U &reply) {
    constexpr bool isEmptyMsg = detail::is_same_v<detail::decay_t<T>, EmptyMessage_t>;
    constexpr bool isEmptyRpl = detail::is_same_v<detail::decay_t<U>, EmptyMessage_t>;
    static_assert(alignof(T) >= alignof(unsigned) || isEmptyMsg, "Unaligned send msg");
    static_assert(alignof(U) >= alignof(unsigned) || isEmptyRpl, "Unaligned send reply");
    int a0 = tid.underlying();
    const void *a1 = isEmptyMsg ? nullptr : &msg;
    int a2 = isEmptyMsg ? 0 : sizeof(T);
    void *a3 = isEmptyRpl ? nullptr : &reply;
    int a4 = isEmptyRpl ? 0 : sizeof(U);
    return detail::send(a0, a1, a2, a3, a4);
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

namespace detail {
inline int receive(Tid *a0, void *a1, int a2) {
    SYSCALL3R(kernel::Syscall::Receive);
    return ret;
}
}
template <typename T>
int receive(Tid *a0, T &msg) {
    constexpr bool isEmpty = detail::is_same_v<detail::decay_t<T>, EmptyMessage_t>;
    static_assert(alignof(T) >= alignof(unsigned) || isEmpty, "Unaligned receive");
    void *a1 = isEmpty ? nullptr : &msg;
    int a2 = isEmpty ? 0 : sizeof(T);
    return detail::receive(a0, a1, a2);
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
int reply(Tid tid, T &msg) {
    constexpr bool isEmpty = detail::is_same_v<detail::decay_t<T>, EmptyMessage_t>;
    static_assert(alignof(T) >= alignof(unsigned) || isEmpty, "Unaligned reply");
    int a0 = tid.underlying();
    const void *a1 = isEmpty ? nullptr : &msg;
    int a2 = isEmpty ? 0 : sizeof(T);
    return detail::reply(a0, a1, a2);
}
}

#endif // USER_ITC_H__INCLUDED
