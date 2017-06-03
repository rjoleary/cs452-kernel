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
    ASSERT(send(NS_TID, msg, rply) == sizeof(rply));
    return -static_cast<int>(rply.error);
}

Tid whoIs(Names name) {
    Message msg;
    msg.type = MsgType::WhoIs;
    msg.name = name;
    Reply rply;
    ASSERT(send(NS_TID, msg, rply) == sizeof(rply));
    ASSERT(rply.error == Error::Ok);
    return rply.tid;
}

void nsMain() {
    Tid map[static_cast<unsigned>(Names::LastName)];
    for (int i = 0; i < static_cast<int>(Names::LastName); ++i)
        map[i] = INVALID_TID;

    for (;;) {
        Tid tid;
        Message msg;
        Reply rply = {Error::Ok};
        ASSERT(receive(&tid, msg) == sizeof(msg));

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
                ASSERT(false);
            }
        }

        ASSERT(reply(tid, rply) == 0);
    }
}
}
