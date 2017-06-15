#include <def.h>
#include <err.h>
#include <event.h>
#include <itc.h>
#include <std.h>
#include <task.h>
#include <ns.h>
#include <heap.h>

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

struct DelayData {
    int tick;
    Tid tid;
};

struct DelayDataComp {
    bool operator()(const DelayData &lhs, const DelayData &rhs) const {
        return lhs.tick < rhs.tick;
    }
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
    Heap<NUM_TD, DelayData, DelayDataComp> minheap;

    void clockNotifier();
    ASSERT(create(Priority(30), clockNotifier) >= 0);

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::Notify: {
                counter++;
                ASSERT(reply(tid, EmptyMessage) == 0);
                while (!minheap.empty() && minheap.peek().tick <= counter) {
                    const auto &data = minheap.pop();
                    ASSERT(reply(data.tid, EmptyMessage) == 0);
                }
                break;
            }

            case MsgType::Delay: {
                minheap.push({counter + msg.ticks, tid});
                break;
            }

            case MsgType::Time: {
                Reply rply;
                rply.ticks = counter;
                ASSERT(reply(tid, rply) == 0);
                break;
            }

            case MsgType::DelayUntil: {
                if (counter <= msg.ticks) {
                    ASSERT(reply(tid, EmptyMessage) == 0);
                    break;
                }
                minheap.push({msg.ticks, tid});
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
    Message notify{MsgType::Notify};
    for (;;) {
        ASSERT(awaitEvent(Event::PeriodicTimer) >= 0);
        ASSERT(send(clockTid, notify, EmptyMessage) == 0);
    }
}
}
