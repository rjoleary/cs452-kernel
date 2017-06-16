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
    char data;
};

struct alignas(4) Reply {
    char data;
};

struct Uart1Traits {
    static constexpr auto &txServer = names::Uart1TxServer;
    static constexpr auto &rxServer = names::Uart1RxServer;
    static constexpr auto txEvent = Event::Uart1Tx;
    static constexpr auto rxEvent = Event::Uart1Rx;
    static constexpr auto dataReg = UART1_BASE + UART_DATA_OFFSET;
    static constexpr auto flagReg = UART1_BASE + UART_FLAG_OFFSET;
    static void checkModem() {
        // TODO: this doesn't actually check the modem
        for (volatile int i = 0; i < 100000; i++);
    }
};

struct Uart2Traits {
    static constexpr auto &txServer = names::Uart2TxServer;
    static constexpr auto &rxServer = names::Uart2RxServer;
    static constexpr auto txEvent = Event::Uart2Tx;
    static constexpr auto rxEvent = Event::Uart2Rx;
    static constexpr auto dataReg = UART2_BASE + UART_DATA_OFFSET;
    static constexpr auto flagReg = UART2_BASE + UART_FLAG_OFFSET;
    static void checkModem() {}
};

template <typename T>
void txNotifierMain() {
    auto serverTid = Tid(whoIs(T::txServer));
    Message message{MsgType::NotifyTx};
    for (;;) {
        Reply toPrint;
        ASSERT(send(serverTid, message, toPrint) == sizeof(toPrint));
        for (;;) {
            if (awaitEvent(T::txEvent) >= 0) {
                if (!(*(volatile unsigned*)(T::flagReg) & TXFF_MASK)) {
                    T::checkModem();
                    *(volatile unsigned*)(T::dataReg) = toPrint.data;
                    break;
                }
            }
        }
    }
}

template <typename T>
void rxNotifierMain() {
    auto serverTid = Tid(whoIs(T::rxServer));
    Message message{MsgType::NotifyRx};
    for (;;) {
        auto ch = awaitEvent(T::rxEvent);
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
template <typename T>
void txMain() {
    // Register with the nameserver.
    ASSERT(registerAs(T::txServer) == 0);

    // Create notifier.
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1), txNotifierMain<T>) >= 0);

    // Buffers for asynchronicity
    typedef CircularBuffer<char, 2048> CharBuffer;
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

template <typename T>
void rxMain() {
    // Register with the nameserver.
    ASSERT(registerAs(T::rxServer) == 0);

    // Create notifier.
    ASSERT(create(Priority(PRIORITY_MAX.underlying()-1), rxNotifierMain<T>) >= 0);

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

// Export template functions.
void (*uart1TxMain)() = txMain<Uart1Traits>;
void (*uart1RxMain)() = rxMain<Uart1Traits>;
void (*uart2TxMain)() = txMain<Uart2Traits>;
void (*uart2RxMain)() = rxMain<Uart2Traits>;
}
