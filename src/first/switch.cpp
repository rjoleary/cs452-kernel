#include <switch.h>
#include <ns.h>
#include <circularbuffer.h>
#include <itc.h>
#include <bwio.h>
#include <clock.h>
#include <def.h>

void savecur();
void restorecur();
void setpos(unsigned, unsigned);

namespace {
constexpr ctl::Name SwitchServ = {"SSwitch"};

enum class MsgType {
    Ready, Update
};

struct Message {
    MsgType type;
    int sw;
    char dir;
};

void notifMain() {
    auto name = SwitchServ;
    name.data[0] = 'N';
    ~ctl::registerAs(name);
    auto ss = whoIs(SwitchServ).asValue();
    auto clock = whoIs(ctl::names::ClockServer).asValue();
    Message msg;
    for (;;) {
        msg.type = MsgType::Ready;
        ~ctl::send(ss, msg, msg);
        bwputc(COM1, msg.dir == 'C' ? 34: 33);
        bwputc(COM1, msg.sw);
        flush(COM1);

        delay(clock, 15);

        bwputc(COM1, 32);
        flush(COM1);
    }
}

inline int toIdx(int sw) {
    if (1 <= sw && sw <= 18)
        return sw - 1;
    else
        return sw - 153 + 18;
}

void updateGui(int sw, char dir) {
    int idx = toIdx(sw);
    int x = idx%11 + 4;
    int y = 31 + idx/11*7;
    savecur();
    setpos(x, y);
    bwputc(COM2, dir);
    restorecur();
    flush(COM2);
}

} // unnamed namespace

void setupSwitches() {
    for (int i = 1; i <= 18; ++i)
        cmdSetSwitch(i, 'C');
    cmdSetSwitch(153, 'C');
    cmdSetSwitch(154, 'S');
    cmdSetSwitch(155, 'C');
    cmdSetSwitch(156, 'S');
}

void cmdSetSwitch(int sw, char dir) {
    if ((sw < 1 || 18 < sw) && (sw < 153 || 156 < sw)) {
        bwputstr(COM2, "Error: switch number must be [1..18] or [153..156]\r\n");
        return;
    }
    auto ss = whoIs(SwitchServ).asValue();
    
    ~send(ss, Message{MsgType::Update, sw, dir}, ctl::EmptyMessage);
}

void switchMan() {
    constexpr auto NumSwitches = 22;
    ~ctl::registerAs(SwitchServ);
    char states[NumSwitches];
    for (int i = 0; i < NumSwitches; ++i) states[i] = 'U';
    ~create(ctl::Priority(30), notifMain);
    auto notifier = ctl::INVALID_TID;

    ctl::CircularBuffer<Message, NumSwitches> queue;
    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
        case MsgType::Update:
            if (states[toIdx(msg.sw)] != msg.dir) {
                if (!(notifier == ctl::INVALID_TID)) {
                    ~reply(notifier, msg);
                    notifier = ctl::INVALID_TID;
                    states[toIdx(msg.sw)] = msg.dir;
                    updateGui(msg.sw, msg.dir);
                }
                else {
                    queue.push(msg);
                }
            }
            ~reply(tid, ctl::EmptyMessage);
            break;
        case MsgType::Ready:
            if (queue.empty()) {
                notifier = tid;
            }
            else {
                const auto& msgToSend = queue.pop();
                ~reply(tid, msgToSend);
                states[toIdx(msgToSend.sw)] = msgToSend.dir;
                updateGui(msgToSend.sw, msgToSend.dir);
            }
            break;
        }
    }
}

