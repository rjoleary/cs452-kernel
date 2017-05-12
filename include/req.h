#ifndef REQ_H__INCLUDED
#define REQ_H__INCLUDED

enum RequestId {
    REQ_CREATE = 1,
    REQ_MYTID,
    REQ_MYPARENTID,
    REQ_PASS,
    REQ_EXIT,
    REQ_DESTROY,
    REQ_SEND,
    REQ_RECEIVE,
    REQ_REPLY,
    REG_AWAITEVENT,
};

struct Request {
    enum RequestId id;
    // Up to five arguments.
    unsigned a1, a2, a3, a4, a5;
};

#endif // REQ_H__INCLUDED
