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
    ReverseWhoIs,
};

struct Message {
    MsgType type;
    union {
        Name name;
        Tid tid;
    };
};

struct Reply {
    Error error;
    union {
        Name name;
        Tid tid;
    };
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
    if (rply.error != Error::Ok) {
        return Tid(-int(rply.error)); // TODO: lol type safety
    }
    return rply.tid;
}

int reverseWhoIs(Tid tid, Name *name) {
    Message msg;
    msg.type = MsgType::ReverseWhoIs;
    msg.tid = tid;
    Reply rply;
    ASSERT(send(NS_TID, msg, rply) == sizeof(rply));
    *name = rply.name;
    return -int(rply.error); // TODO: lol type safety
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
        if (msg.type != MsgType::ReverseWhoIs) {
            for (unsigned i = 0; i < mapSize; i++) {
                // TODO: overload comparision
                if (memcmp(msg.name.data, map[i].name.data, sizeof(msg.name.data)) == 0) {
                    idx = i;
                    break;
                }
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

            case MsgType::ReverseWhoIs: {
                for (unsigned i = 0; i < mapSize; i++) {
                    if (msg.tid == map[i].tid) {
                        idx = i;
                        break;
                    }
                }
                if (idx != -1) {
                    rply.name = map[idx].name;
                } else {
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
