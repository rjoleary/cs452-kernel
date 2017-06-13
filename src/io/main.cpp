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

template <Source src, Names server>
void genericNotifierMain() {
    auto serverTid = Tid(whoIs(server));
    for (;;) {
        Message message;
        int c = awaitEvent(src);

        // Ignore corrupt data.
        if (c >= 0) {
            auto interruptType = 
                *(volatile unsigned*)(UART2_BASE + UART_INTR_OFFSET);
            if (interruptType & RIS_MASK) {
                message.type = MsgType::NotifyRx;
                message.data = *(volatile unsigned*)(UART2_BASE + UART_DATA_OFFSET) & 0xff;
            }
            else if (interruptType & TIS_MASK) {
                message.type = MsgType::NotifyTx;
                message.data = 0;
            }
            // Ignore other interrupt reasons
            else continue;
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
        genericNotifierMain<Source::INT_UART2, Names::IoServer>) > 0);

    // Buffers for asynchronicity
    typedef CircularBuffer<char,1024> CharBuffer;
    CharBuffer rxQueue, txQueue;
    bool txFull = false;
    CircularBuffer<Tid, NUM_TD> blockQueue;

    for (;;) {
        Tid tid;
        Message msg;
        ASSERT(receive(&tid, msg) == sizeof(msg));

        switch (msg.type) {
            case MsgType::NotifyRx: {
                // Reply immediately so we don't miss any interrupts.
                ASSERT(reply(tid, EmptyMessage) == 0);
                if (blockQueue.empty()) {
                    rxQueue.push(msg.data);
                } else {
                    ASSERT(rxQueue.empty());
                    Reply rply{msg.data};
                    ASSERT(reply(blockQueue.pop(), rply) == 0);
                }
                break;
            }

            case MsgType::NotifyTx: {
                // Reply immediately so we don't miss any interrupts.
                ASSERT(reply(tid, EmptyMessage) == 0);
                if (txQueue.empty()) {
                    txFull = false;
                } else {
                    ASSERT(txFull);
                    *(volatile unsigned*)(UART2_BASE + UART_DATA_OFFSET) = txQueue.pop();
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
                if (txFull) {
                    txQueue.push(msg.data);
                } else {
                    txFull = true;
                    *(volatile unsigned*)(UART2_BASE + UART_DATA_OFFSET) = msg.data;
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
}
