#include <../panic.h>
#include <def.h>
#include <err.h>
#include <itc.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <bwio.h>

namespace {
enum class MsgType {
    Register,
    WhoIs,
};

struct Message {
    MsgType type;
    char name[STR_SIZE] = {};
};

struct Reply {
    Tid tid;
};
}

namespace ctl {
int registerAs(const char *name) {
    Message msg;
    msg.type = MsgType::Register;
    // Names longer than name STR_SIZE-1 are truncated.
    __builtin_strncpy(msg.name, name, sizeof(msg.name) - 1);
    Reply rply;
    int err = send(NS_TID, msg, rply);
    if (err < 0) {
        return err;
    }
    return rply.tid;
}

int whoIs(const char *name) {
    Message msg;
    msg.type = MsgType::WhoIs;
    // Names longer than name STR_SIZE-1 are truncated.
    __builtin_strncpy(msg.name, name, sizeof(msg.name) - 1);
    Reply rply;
    int err = send(NS_TID, msg, rply);
    if (err < 0) {
        return err;
    }
    return rply.tid;
}

void nsMain() {
    // Mapping of strings to tids.
    struct {
        char name[STR_SIZE];
        Tid tid = -1;
    } map[NS_MAX];

    while (1) {
        Tid tid;
        Message msg;
        Reply rply = {static_cast<int>(Error::Ok)};
        if (receive(&tid, &msg, sizeof(msg)) != sizeof(msg)) {
            PANIC("message with wrong size");
        }

        switch (msg.type) {
            case MsgType::Register: {
                for (auto &kv : map) {
                    if (kv.tid == -1) {
                        __builtin_memcpy(kv.name, msg.name, sizeof(kv.name));
                        kv.tid = tid;
                        goto doReply;
                    }
                }
                rply.tid = -static_cast<int>(Error::NoRes);
                break;
            }

            case MsgType::WhoIs: {
                for (const auto &kv : map) {
                    if (memcmp(kv.name, msg.name, sizeof(kv.name)) == 0) {
                        rply.tid = kv.tid;
                        goto doReply;
                    }
                }
                rply.tid = -static_cast<int>(Error::BadArg);
                break;
            }

            default: {
                PANIC("unknown message type");
            }
        }

        doReply:
        if (int err = reply(tid, &rply, sizeof(rply))) {
            bwprintf(COM2, "%d\r\n", err);
            PANIC("ns reply returned error");
        }
    }
}
}
