#include <def.h>
#include <err.h>
#include <event.h>
#include <itc.h>
#include <std.h>
#include <task.h>
#include <ns.h>

namespace ctl {
namespace {
enum class MsgType {
    Notify,
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
}

int delay(Tid tid, int ticks) {
    ASSERT(ticks > 0);

    Message msg;
    msg.type = MsgType::Delay;
    msg.ticks = ticks;
    int err = send(tid, msg, EmptyMessage);
    if (err == -static_cast<int>(Error::InvId)) {
        return err;
    }
    ASSERT(err == 0);
    return static_cast<int>(Error::Ok);
}

int time(Tid tid) {
    Message msg;
    msg.type = MsgType::Time;
    Reply rply;
    int err = send(tid, msg, rply);
    if (err == -static_cast<int>(Error::InvId)) {
        return err;
    }
    ASSERT(err == sizeof(rply));
    return rply.ticks;
}

int delayUntil(Tid tid, int ticks) {
    ASSERT(ticks > 0);

    Message msg;
    msg.type = MsgType::DelayUntil;
    msg.ticks = ticks;
    int err = send(tid, msg, EmptyMessage);
    if (err == -static_cast<int>(Error::InvId)) {
        return err;
    }
    ASSERT(err == 0);
    return static_cast<int>(Error::Ok);
}

void clockMain() {
    int counter = 0;
    ASSERT(registerAs(Names::ClockServer) == 0);
    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::Notify: {
                counter++;
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            case MsgType::Delay: {
                // TODO: implement
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            case MsgType::Time: {
                Reply rply;
                rply.ticks = counter;
                ASSERT(reply(tid, rply) == 0);
                break;
            }

            case MsgType::DelayUntil: {
                // TODO: implement
                ASSERT(reply(tid, EmptyMessage));
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}

void clockNotifier() {
    auto clockTid = Tid(whoIs(Names::ClockServer));
    Message notif{MsgType::Notify};
    for (;;) {
        ASSERT(awaitEvent(InterruptSource::TC1UI) >= 0);
        ASSERT(send(clockTid, notif, EmptyMessage) == 0);
    }
}
}
