#include <def.h>
#include <err.h>
#include <itc.h>
#include <std.h>
#include <task.h>

enum class MsgType {
    Delay,
    Time,
    DelayUntil,
};

struct Message {
    MsgType type;
    int ticks;
};

struct Reply {
    int ticks;
};

int delay(ctl::Tid tid, int ticks) {
    if (ticks <= 0) {
        return -static_cast<int>(ctl::Error::BadArg);
    }

    Message msg;
    msg.type = MsgType::Delay;
    msg.ticks = ticks;
    int err = send(tid, msg, ctl::EmptyMessage);
    if (err == -static_cast<int>(ctl::Error::InvId)) {
        return err;
    }
    ASSERT(err == 0);
    return static_cast<int>(ctl::Error::Ok);
}

int time(ctl::Tid tid) {
    Message msg;
    msg.type = MsgType::Time;
    Reply rply;
    int err = send(tid, msg, rply);
    if (err == -static_cast<int>(ctl::Error::InvId)) {
        return err;
    }
    ASSERT(err == sizeof(rply));
    return static_cast<int>(ctl::Error::Ok);
}

int delayUntil(ctl::Tid tid, int ticks) {
    if (ticks <= 0) {
        return -static_cast<int>(ctl::Error::BadArg);
    }

    Message msg;
    msg.type = MsgType::DelayUntil;
    msg.ticks = ticks;
    int err = send(tid, msg, ctl::EmptyMessage);
    if (err == -static_cast<int>(ctl::Error::InvId)) {
        return err;
    }
    ASSERT(err == 0);
    return static_cast<int>(ctl::Error::Ok);
}

void clockMain() {
    for (;;) {
        ctl::Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::Delay: {
                // TODO: implement
                ASSERT(reply(tid, ctl::EmptyMessage));
                break;
            }

            case MsgType::Time: {
                Reply rply;
                // TODO: implement
                ASSERT(reply(tid, rply));
                break;
            }

            case MsgType::DelayUntil: {
                // TODO: implement
                ASSERT(reply(tid, ctl::EmptyMessage));
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}
