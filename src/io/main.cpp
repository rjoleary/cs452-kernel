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
    static constexpr auto taskBufferSize = 8;
    static constexpr auto flushOnNewline = false;
    static bool cts() { 
        return *(volatile unsigned*)(flagReg) & CTS_MASK;
    }

    struct BlkInfo {
        Tid tid;
        size_t size;
    };

    CircularBuffer<BlkInfo, NUM_TD> blkInfo;
    unsigned sentData = 0;
    void sent() {
        sentData++;
        if (blkInfo.front().size == sentData) {
            ~reply(blkInfo.pop().tid, EmptyMessage);
            sentData = 0;
        }
    }
    void flushed(Tid tid, size_t size) {
        if (size != 0)
            blkInfo.push({tid, size});
    }
    void respond(Tid tid, bool flushed) {
        if (!flushed)
            ~reply(tid, EmptyMessage);
    }
};

struct Uart2Traits {
    static constexpr auto &txServer = names::Uart2TxServer;
    static constexpr auto &rxServer = names::Uart2RxServer;
    static constexpr auto txEvent = Event::Uart2Tx;
    static constexpr auto rxEvent = Event::Uart2Rx;
    static constexpr auto dataReg = UART2_BASE + UART_DATA_OFFSET;
    static constexpr auto flagReg = UART2_BASE + UART_FLAG_OFFSET;
    static constexpr auto taskBufferSize = 64;
    static constexpr auto flushOnNewline = true;
    static bool cts() { return true; }

    void sent() {}
    void flushed(Tid, size_t) {}
    void respond(Tid tid, bool) { ~reply(tid, EmptyMessage); }
};

template <typename T>
void txNotifierMain() {
    // Register
    Name name = T::txServer;
    name.data[0] = 'N';
    ~registerAs(name);

    auto serverTid = whoIs(T::txServer).asValue();
    Message message{MsgType::NotifyTx};
    for (;;) {
        Reply toPrint;
        ~send(serverTid, message, toPrint);
        for (;;) {
            if (awaitEvent(T::txEvent) >= 0) {
                if (!(*(volatile unsigned*)(T::flagReg) & TXBUSY_MASK)
                        && !(*(volatile unsigned*)(T::flagReg) & TXFF_MASK)
                        && T::cts()) {
                    *(volatile unsigned*)(T::dataReg) = toPrint.data;
                    break;
                }
            }
        }
    }
}

template <typename T>
void rxNotifierMain() {
    // Register
    Name name = T::rxServer;
    name.data[0] = 'N';
    ~registerAs(name);

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

    T trait;

    // Value for blocking tx notifier.
    Tid txFull = INVALID_TID;
    auto pushData = [&](int idx) {
        while (!buffers[idx].empty()) {
            if (txFull == INVALID_TID) {
                queue.push(buffers[idx].pop());
            } else {
                ~reply(txFull, Reply{buffers[idx].pop()});
                trait.sent();
                txFull = INVALID_TID;
            }
        }
    };

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
                    trait.sent();
                }

                break;
            }

            case MsgType::PutC: {
                int idx = tid.underlying();
                
                // Add to buffer.
                buffers[idx].push(msg.data);

                bool flushed = false;
                // Check if buffer needs flushing.
                if (buffers[idx].full() || (T::flushOnNewline && msg.data == '\n')) {
                    trait.flushed(tid, buffers[idx].size());
                    pushData(idx);
                    flushed = true;
                }

                trait.respond(tid, flushed);
                break;
            }

            case MsgType::Flush: {
                int idx = tid.underlying();
                trait.flushed(tid, buffers[idx].size());
                pushData(idx);
                trait.respond(tid, true);
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
