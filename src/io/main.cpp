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
#include <ts7200.h>

using namespace ctl;

namespace {
enum class MsgType {
    NotifyRx,
    NotifyTx,
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

template <Event src, Names server>
void genericRxNotifierMain() {
    auto serverTid = Tid(whoIs(server));
    Message message{MsgType::NotifyRx};
    for (;;) {
        message.data = awaitEvent(src, 0);
        if (message.data < 0) {
            // Ignore corrupt data
            continue;
        }
        ASSERT(send(serverTid, message, EmptyMessage) == 0);
    }
}

template <Event src, Names server>
void genericTxNotifierMain() {
    auto serverTid = Tid(whoIs(server));
    Message message{MsgType::NotifyTx};
    for (;;) {
        Reply rply;
        ASSERT(send(serverTid, message, rply) == sizeof(rply));
        awaitEvent(src, rply.data);
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

template <Names server, Event rx, Event tx>
void ioMain() {
    // Register with the nameserver.
    ASSERT(registerAs(server) == 0);

    // Create notifiers.
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1),
        genericRxNotifierMain<rx, server>) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1),
        genericTxNotifierMain<tx, server>) >= 0);

    // Buffers for asynchronicity
    typedef CircularBuffer<char,512> CharBuffer;
    CharBuffer rxQueue, txQueue;
    Tid txFull = INVALID_TID;
    CircularBuffer<Tid, NUM_TD> blockQueue;

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::NotifyRx: {
                if (blockQueue.empty()) {
                    rxQueue.push(msg.data);
                } else {
                    ASSERT(rxQueue.empty());
                    Reply rply{msg.data};
                    ASSERT(reply(blockQueue.pop(), rply) == 0);
                }
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            case MsgType::NotifyTx: {
                if (txQueue.empty()) {
                    txFull = tid;
                } else {
                    Reply rply{txQueue.pop()};
                    ASSERT(reply(tid, rply) == 0);
                    txFull = INVALID_TID;
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
                if (txFull == INVALID_TID) {
                    txQueue.push(msg.data);
                } else {
                    Reply rply{msg.data};
                    ASSERT(reply(txFull, rply) == 0);
                    txFull = INVALID_TID;
                }
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}

void (*ioMainUart1)() = io::ioMain<Names::IoServerUart1, Event::Uart1Rx, Event::Uart1Tx>;
void (*ioMainUart2)() = io::ioMain<Names::IoServerUart2, Event::Uart2Rx, Event::Uart2Tx>;
}
