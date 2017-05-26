#include <../panic.h>
#include <def.h>
#include <err.h>
#include <itc.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <bwio.h>

namespace ctl {
namespace {
enum class MsgType {
    Register,
    WhoIs,
};

struct Message {
    MsgType type;
    Names name;
};

struct Reply {
    Error error;
    Tid tid;
};
}

int registerAs(Names name) {
    Message msg;
    msg.type = MsgType::Register;
    msg.name = name;

    Reply rply;
    if (send(NS_TID, msg, rply) < 0)
        PANIC("registerAs send failed");
    return -static_cast<int>(rply.error);
}

int whoIs(Names name) {
    Message msg;
    msg.type = MsgType::WhoIs;
    msg.name = name;
    Reply rply;
    if (send(NS_TID, msg, rply) < 0)
        PANIC("whoIs send failed");
    if (rply.error != Error::Ok)
        return -static_cast<int>(rply.error);
    return rply.tid.underlying();
}

void nsMain() {
    Tid map[static_cast<unsigned>(Names::LastName)];
    for (int i = 0; i < static_cast<int>(Names::LastName); ++i)
        map[i] = INVALID_TID;

    while (1) {
        Tid tid;
        Message msg;
        Reply rply = {Error::Ok};
        if (receive(&tid, msg) != sizeof(msg)) {
            PANIC("message with wrong size");
        }

        auto idx = static_cast<unsigned>(msg.name);
        switch (msg.type) {
            case MsgType::Register: {
                if (map[idx] == INVALID_TID)
                    map[idx] = tid;
                else
                    rply.error = Error::NoRes;
                break;
            }

            case MsgType::WhoIs: {
                if (map[idx] == INVALID_TID) 
                    rply.error = Error::BadArg;
                else 
                    rply.tid = map[idx];
                break;
            }

            default: {
                PANIC("unknown message type");
            }
        }

        if (int err = reply(tid, rply)) {
            bwprintf(COM2, "%d\r\n", err);
            PANIC("ns reply returned error");
        }
    }
}
}
