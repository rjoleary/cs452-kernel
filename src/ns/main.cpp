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
    Name name;
};

struct Reply {
    Error error;
    Tid tid;
};
}

int registerAs(Name name) {
    Message msg;
    msg.type = MsgType::Register;
    msg.name = name;

    Reply rply;
    ASSERT(send(NS_TID, msg, rply) == sizeof(rply));
    return -static_cast<int>(rply.error);
}

Tid whoIs(Name name) {
    Message msg;
    msg.type = MsgType::WhoIs;
    msg.name = name;
    Reply rply;
    ASSERT(send(NS_TID, msg, rply) == sizeof(rply));
    ASSERT(rply.error == Error::Ok);
    return rply.tid;
}

void nsMain() {
    // Map from names to servers.
    struct Entry {
        Name name;
        Tid tid = INVALID_TID;
    };
    Entry map[NUM_TD];
    unsigned mapSize = 0;

    for (;;) {
        Tid tid;
        Message msg;
        Reply rply = {Error::Ok};
        ASSERT(receive(&tid, msg) == sizeof(msg));

        int idx = -1;
        for (unsigned i = 0; i < mapSize; i++) {
            if (memcmp(msg.name.data, map[i].name.data, sizeof(msg.name.data)) == 0) {
                idx = i;
                break;
            }
        }

        switch (msg.type) {
            case MsgType::Register: {
                if (mapSize >= NUM_TD || idx != -1) {
                    // TODO: is this up to spec? or should it overwrite
                    rply.error = Error::NoRes;
                } else {
                    map[mapSize].name = msg.name;
                    map[mapSize].tid = tid;
                    mapSize++;
                }
                break;
            }

            case MsgType::WhoIs: {
                if (idx != -1)  {
                    rply.tid = map[idx].tid;
                } else  {
                    rply.error = Error::BadArg;
                }
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
