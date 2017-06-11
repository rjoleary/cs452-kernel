#include <bwio.h>
#include <clock.h>
#include <def.h>
#include <event.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <itc.h>
#include <io.h>

// Forward declaration.
void idleMain();
namespace io {
void ioMain();
}

namespace ctl {
void nsMain();
void clockNotifier();
void clockMain();

namespace {
struct Message {
    Priority p; // priority
    int t; // delay time
    int n; // number of delays
};

void clientMain() {
    Message rply;
    ASSERT(send(myParentTid(), EmptyMessage, rply) == sizeof(rply));
    auto clockTid = Tid(whoIs(Names::ClockServer));
    for (int i = 0; i < rply.n; i++) {
        delay(clockTid, rply.t);
        bwprintf(COM2, "Tid: %d, MyDelay: %d, DelayNum: %d\r\n",
            myTid(), rply.t, i + 1);
    }

    // Send exit message to parent. Does not return
    send(myParentTid(), EmptyMessage, EmptyMessage);
}
}

void firstMain() {
    ASSERT(Tid(create(PRIORITY_MIN, idleMain)) == IDLE_TID);
    ASSERT(Tid(create(Priority(PRIORITY_MAX.underlying() - 2), nsMain)) == NS_TID);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 1), io::ioMain) >= 0);
    ASSERT(create(Priority(30), clockMain) >= 0);
    ASSERT(create(Priority(30), clockNotifier) >= 0);

    Message msgs[] = {
        {Priority{6}, 10, 20},
        {Priority{5}, 23, 9},
        {Priority{4}, 33, 6},
        {Priority{3}, 71, 3},
    };

    // Create tasks.
    for (const auto &msg : msgs) {
        auto ret = create(msg.p, clientMain);
        ASSERT(ret > 0);
    }

    // Send parameters.
    for (const auto &msg : msgs) {
        Tid tid;
        ASSERT(receive(&tid, EmptyMessage) == 0);
        ASSERT(reply(tid, msg) == 0);
    }

    // Block until the tasks finish.
    for (const auto &msg : msgs) {
        (void)msg;
        Tid tid;
        ASSERT(receive(&tid, EmptyMessage) == 0);
    }

    // Echo
    Tid io = whoIs(Names::IoServer);
    ASSERT(io.underlying() >= 0);
    for (;;) {
        int c = io::getc(io, COM1);
        ASSERT(c >= 0);
        ASSERT(io::putc(io, COM2, c) >= 0);
    }
}
}
