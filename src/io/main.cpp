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

    typedef CircularBuffer<char,1024> CharBuffer;
    CharBuffer rxQueue;
    CircularBuffer<Tid, NUM_TD> blockQueue;

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::Notify: {
                // Reply immediately so we don't miss any interrupts.
                ASSERT(reply(tid, EmptyMessage) == 0);
                if (blockQueue.empty()) {
                    rxQueue.push(msg.data);
                } else {
                    Reply rply{msg.data};
                    ASSERT(reply(blockQueue.pop(), rply) == 0);
                }
                break;
            }

            case MsgType::GetC: {
                if (rxQueue.empty()) {
                    blockQueue.push(tid);
                } else {
                    Reply rply{rxQueue.pop()};
                    ASSERT(reply(tid, rply) == 0);
                }
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
