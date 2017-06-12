#include <io.h>

#include <bwio.h>
#include <circularbuffer.h>
#include <def.h>
#include <err.h>
#include <event.h>
#include <event.h>
#include <itc.h>
#include <ns.h>
#include <task.h>

using namespace ctl;

namespace {
typedef CircularBuffer<char,128> CharBuffer;

enum MsgType {
    Notify,
    GetC,
    PutC,
};

struct Message {
    MsgType type;
    int uart;
    char data;
};

struct alignas(4) Reply {
    char data;
};

template <Source src, int uart, Names server>
void genericNotifierMain() {
    auto serverTid = Tid(whoIs(server));
    Message message{MsgType::Notify, uart};
    for (;;) {
        int c = awaitEvent(src);
        // Ignore corrupt data.
        if (c >= 0) {
            message.data = c;
            ASSERT(send(serverTid, message, EmptyMessage) == 0);
        }
    }
}
}

namespace io {
int getc(Tid tid, int uart) {
    Message msg;
    msg.type = MsgType::GetC;
    msg.uart = uart;
    Reply rply;
    int err = send(tid, msg, rply);
    if (err == -static_cast<int>(Error::InvId)) {
        return err;
    }
    ASSERT(err == sizeof(rply));
    return rply.data;
}

int putc(Tid tid, int uart, char ch) {
    Message msg;
    msg.type = MsgType::PutC;
    msg.uart = uart;
    msg.data = ch;
    Reply rply;
    int err = send(tid, msg, rply);
    if (err == -static_cast<int>(Error::InvId)) {
        return err;
    }
    ASSERT(err == 0);
    return static_cast<int>(Error::Ok);
}

void ioMain() {
    // Register with the nameserver.
    ASSERT(registerAs(Names::IoServer) == 0);

    // Create notifiers.
    ASSERT(create(PRIORITY_MAX,
        genericNotifierMain<Source::UART2RXINTR2, COM2, Names::IoServer>) > 0);
    //ASSERT(create(PRIORITY_MAX,
    //    genericNotifierMain<Source::UART2TXINTR2, Names::IoServer>) > 0);

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::Notify: {
                char data = msg.data;
                bwputc(COM2, data);
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            case MsgType::GetC: {
                // TODO
                break;
            }

            case MsgType::PutC: {
                // TODO
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}
}
