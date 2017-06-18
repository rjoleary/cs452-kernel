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
    Flush,
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
    static constexpr auto taskBufferSize = 2;
    static constexpr auto flushOnNewline = false;
};

struct Uart2Traits {
    static constexpr auto &txServer = names::Uart2TxServer;
    static constexpr auto &rxServer = names::Uart2RxServer;
    static constexpr auto txEvent = Event::Uart2Tx;
    static constexpr auto rxEvent = Event::Uart2Rx;
    static constexpr auto dataReg = UART2_BASE + UART_DATA_OFFSET;
    static constexpr auto flagReg = UART2_BASE + UART_FLAG_OFFSET;
    static void checkModem() {}
    static constexpr auto taskBufferSize = 64;
    static constexpr auto flushOnNewline = true;
};

template <typename T>
void txNotifierMain() {
    auto serverTid = whoIs(T::txServer).asValue();
    Message message{MsgType::NotifyTx};
    for (;;) {
        Reply toPrint;
        ~send(serverTid, message, toPrint);
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
    auto serverTid = whoIs(T::rxServer).asValue();
    Message message{MsgType::NotifyRx};
    for (;;) {
        auto ch = awaitEvent(T::rxEvent);
        if (ch >= 0) {
            message.data = ch;
            ~send(serverTid, message, EmptyMessage);
        }
    }
}
}

namespace io {
ErrorOr<int> getc(Tid tid) {
    Message msg;
    msg.type = MsgType::GetC;
    Reply rply;
    auto err = send(tid, msg, rply);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    int data = rply.data;
    return err.replace(data);
}

ErrorOr<void> putc(Tid tid, char ch) {
    Message msg;
    msg.type = MsgType::PutC;
    msg.data = ch;
    Reply rply;
    auto err = send(tid, msg, rply);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    return err;
}

ErrorOr<void> flush(Tid tid) {
    Message msg;
    msg.type = MsgType::Flush;
    auto err = send(tid, msg, EmptyMessage);
    ASSERT(!err.isError() || err.asError() == Error::InvId);
    return err;
}

// Handles TX interrupt, used by putc
template <typename T>
void txMain() {
    // Register with the nameserver.
    ~registerAs(T::txServer);

    // Create notifier.
    ~create(Priority(PRIORITY_MAX.underlying()-1), txNotifierMain<T>);

    // Per-task buffers for atomicity.
    CircularBuffer<char, T::taskBufferSize> buffers[NUM_TD];

    // Buffer for asynchronicity.
    CircularBuffer<char, 1024> queue;

    // Value for blocking tx notifier.
    Tid txFull = INVALID_TID;

    for (;;) {
        Tid tid;
        Message msg;
        ~receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NotifyTx: {
                if (queue.empty()) {
                    txFull = tid;
                } else {
                    ~reply(tid, Reply{queue.pop()});
                }

                break;
            }

            case MsgType::PutC: {
                int idx = tid.underlying();
                
                // Add to buffer.
                buffers[idx].push(msg.data);

                // Check if buffer needs flushing.
                if (buffers[idx].full() || (T::flushOnNewline && msg.data == '\n')) {
                    while (!buffers[idx].empty()) {
                        if (txFull == INVALID_TID) {
                            queue.push(buffers[idx].pop());
                        } else {
                            ~reply(txFull, Reply{buffers[idx].pop()});
                            txFull = INVALID_TID;
                        }
                    }
                }

                ~reply(tid, EmptyMessage);
                break;
            }

            case MsgType::Flush: {
                int idx = tid.underlying();
                while (!buffers[idx].empty()) {
                    if (txFull == INVALID_TID) {
                        queue.push(buffers[idx].pop());
                    } else {
                        ~reply(txFull, Reply{buffers[idx].pop()});
                        txFull = INVALID_TID;
                    }
                }
                ~reply(tid, EmptyMessage);
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
    ~registerAs(T::rxServer);

    // Create notifier.
    ~create(Priority(PRIORITY_MAX.underlying()-1), rxNotifierMain<T>);

    // Buffers for asynchronicity
    typedef CircularBuffer<char, 512> CharBuffer;
    CharBuffer queue;
    CircularBuffer<Tid, NUM_TD> blockQueue;

    for (;;) {
        Tid tid;
        Message msg;
        ~receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NotifyRx: {
                if (blockQueue.empty()) {
                    queue.push(msg.data);
                } else {
                    ~reply(blockQueue.pop(), Reply{msg.data});
                }
                ~reply(tid, EmptyMessage);
                break;
            }

            case MsgType::GetC: {
                if (queue.empty()) {
                    blockQueue.push(tid);
                } else {
                    ~reply(tid, Reply{queue.pop()});
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
