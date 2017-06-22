#include <def.h>
#include <err.h>
#include <event.h>
#include <itc.h>
#include <std.h>
#include <task.h>
#include <ns.h>
#include <heap.h>
#include <bwio.h>

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

ErrorOr<void> delay(Tid tid, int ticks) {
    ASSERT(ticks > 0);

    Message msg;
    msg.type = MsgType::Delay;
    msg.ticks = ticks;
    auto err = send(tid, msg, EmptyMessage);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    return err;
}

ErrorOr<int> time(Tid tid) {
    Message msg;
    msg.type = MsgType::Time;
    Reply rply;
    auto err = send(tid, msg, rply);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    return err.replace(rply.ticks);
}

ErrorOr<void> delayUntil(Tid tid, int ticks) {
    ASSERT(ticks > 0);

    Message msg;
    msg.type = MsgType::DelayUntil;
    msg.ticks = ticks;
    auto err = send(tid, msg, EmptyMessage);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    return err;
}

void clockMain() {
    int counter = 0;
    ~registerAs(names::ClockServer);
    Heap<NUM_TD, DelayData, DelayDataComp> minheap;

    void clockNotifier();
    ~create(Priority(30), clockNotifier);

    for (;;) {
        Tid tid;
        Message msg;
        ~receive(&tid, msg);

        switch (msg.type) {
            case MsgType::Notify: {
                counter++;
                ~reply(tid, EmptyMessage);
                while (!minheap.empty() && minheap.peek().tick <= counter) {
                    const auto &data = minheap.pop();
                    ~reply(data.tid, EmptyMessage);
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
                ~reply(tid, rply);
                break;
            }

            case MsgType::DelayUntil: {
                if (msg.ticks <= counter) {
                    ~reply(tid, EmptyMessage);
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
    ~registerAs(Name{"NClock"});
    auto clockTid = whoIs(names::ClockServer).asValue();
    Message notify{MsgType::Notify};
    for (;;) {
        ASSERT(awaitEvent(Event::PeriodicTimer) >= 0);
        ~send(clockTid, notify, EmptyMessage);
    }
}
}
