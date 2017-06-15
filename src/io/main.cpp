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

template <Names Server, Event Ev>
void txNotifierMain() {
    auto serverTid = Tid(whoIs(Server));
    Message message{MsgType::NotifyTx};

    auto data = Ev == Event::Uart1Tx
        ? (volatile unsigned*)(UART1_BASE + UART_DATA_OFFSET)
        : (volatile unsigned*)(UART2_BASE + UART_DATA_OFFSET);
    auto flags = Ev == Event::Uart1Tx
        ? (volatile unsigned*)(UART1_BASE + UART_FLAG_OFFSET)
        : (volatile unsigned*)(UART2_BASE + UART_FLAG_OFFSET);
    for (;;) {
        Reply toPrint;
        ASSERT(send(serverTid, message, toPrint) == sizeof(toPrint));
        for (;;) {
            if (awaitEvent(Ev) >= 0) {
                if (!(*flags & TXFF_MASK)) {
                    *data = toPrint.data;
                    break;
                }
            }
        }
    }
}

template <Names Server, Event Ev>
void rxNotifierMain() {
    auto serverTid = Tid(whoIs(Server));
    Message message{MsgType::NotifyRx};
    for (;;) {
        auto ch = awaitEvent(Ev);
        if (ch >= 0) {
            message.data = ch;
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

// Handles TX interrupt, used by putc
template <Names Server, Event Ev>
void txMain() {
    // Register with the nameserver.
    ASSERT(registerAs(Server) == 0);

    // Create notifiers.
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1),
        txNotifierMain<Server, Ev>) >= 0);

    // Buffers for asynchronicity
    typedef CircularBuffer<char, 512> CharBuffer;
    CharBuffer queue;
    Tid txFull = INVALID_TID;

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::NotifyTx: {
                if (queue.empty()) {
                    txFull = tid;
                } else {
                    ASSERT(reply(tid, Reply{queue.pop()}) == 0);
                }

                break;
            }

            case MsgType::PutC: {
                ASSERT(reply(tid, EmptyMessage) == 0);
                if (txFull == INVALID_TID) {
                    queue.push(msg.data);
                } else {
                    ASSERT(reply(txFull, Reply{msg.data}) == 0);
                    txFull = INVALID_TID;
                }
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}

template void txMain<Names::Uart2Tx, Event::Uart2Tx>();
template void txMain<Names::Uart1Tx, Event::Uart1Tx>();

template <Names Server, Event Ev>
void rxMain() {
    // Register with the nameserver.
    ASSERT(registerAs(Server) == 0);

    // Create notifiers.
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1),
        rxNotifierMain<Server, Ev>) >= 0);

    // Buffers for asynchronicity
    typedef CircularBuffer<char, 512> CharBuffer;
    CharBuffer queue;
    CircularBuffer<Tid, NUM_TD> blockQueue;

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::NotifyRx: {
                if (blockQueue.empty()) {
                    queue.push(msg.data);
                } else {
                    ASSERT(reply(blockQueue.pop(), Reply{msg.data}) == 0);
                }
                ASSERT(reply(tid, EmptyMessage) == 0);
                break;
            }

            case MsgType::GetC: {
                if (queue.empty()) {
                    blockQueue.push(tid);
                } else {
                    ASSERT(reply(tid, Reply{queue.pop()}) == 0);
                }
                break;
            }

            default: {
                ASSERT(false); // unknown message type
            }
        }
    }
}
template void rxMain<Names::Uart2Rx, Event::Uart2Rx>();
template void rxMain<Names::Uart1Rx, Event::Uart1Rx>();

}
