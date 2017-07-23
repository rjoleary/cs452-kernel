#include <switch.h>

#include <bwio.h>
#include <circularbuffer.h>
#include <clock.h>
#include <def.h>
#include <itc.h>
#include <ns.h>

void savecur();
void restorecur();
void setpos(unsigned, unsigned);

namespace {
constexpr ctl::Name SwitchServ = {"SSwitch"};

enum class MsgType {
    Ready, Update, GetData, WaitChange
};

struct Message {
    MsgType type;
    Switch sw;
    SwitchState dir;
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
        bwputc(COM1, msg.dir == SwitchState::Curved ? 34 : 33);
        bwputc(COM1, msg.sw);
        flush(COM1);

        delay(clock, 15);

        bwputc(COM1, 32);
        flush(COM1);
    }
}

const struct {
    char row, col;
    char curved, straight;
} LAYOUT_POS[] = {
    // 1-6
    {11, 5,  '-',  '\\'},
    {13, 7,  '-',  '\\'},
    {15, 9,  '\\', '-'},
    {3,  5,  '-',  '/'},
    {15, 24, '/',  '-'},
    {13, 14, '\\', '-'},

    // 7-12
    {13, 26, '/', '-'},
    {11, 31, '-', '/'},
    {3,  31, '-', '\\'},
    {3,  23, '/', '-'},
    {1,  11, '/', '-'},
    {1,  7,  '/', '-'},

    // 13-18
    {3,  17, '\\', '-'},
    {3,  9,  '-',  '/'},
    {11, 9,  '-',  '\\'},
    {11, 17, '/',  '-'},
    {11, 23, '\\', '-'},
    {15, 16, '\\', '-'},

    // 153-156
    {8, 20, '#', '|'},
    {8, 20, '#', '|'},
    {6, 20, '#', '|'},
    {6, 20, '#', '|'},
};

void updateGui(int sw, SwitchState dir, const SwitchStates &ss) {
    const int idx = SwitchStates::toIdx(sw);
    const auto &layoutPos = LAYOUT_POS[idx];

    char c = dir == SwitchState::Curved ? layoutPos.curved : layoutPos.straight;
    if ((sw == 153 || sw == 154) && (ss[153] != ss[154])) {
        if (ss[153] == SwitchState::Curved) {
            c = '/';
        } else if (ss[154] == SwitchState::Curved) {
            c = '\\';
        }
    } else if ((sw == 155 || sw == 156) && (ss[155] != ss[156])) {
        if (ss[155] == SwitchState::Curved) {
            c = '/';
        } else if (ss[156] == SwitchState::Curved) {
            c = '\\';
        }
    }

    savecur();
    bwputstr(COM2, "\033[1m"); // bold
    setpos(layoutPos.row + 2, layoutPos.col);
    bwputc(COM2, c);
    bwputstr(COM2, "\033[0m"); // unbold
    restorecur();
    flush(COM2);
}

} // anonymous namespace

void setupSwitches() {
    for (int i = 1; i <= 18; ++i) {
        cmdSetSwitch(i, SwitchState::Curved);
    }
    cmdSetSwitch(153, SwitchState::Curved);
    cmdSetSwitch(154, SwitchState::Straight);
    cmdSetSwitch(155, SwitchState::Curved);
    cmdSetSwitch(156, SwitchState::Straight);
}

void cmdSetSwitch(Switch sw, SwitchState dir) {
    if ((sw < 1 || 18 < sw) && (sw < 153 || 156 < sw)) {
        bwputstr(COM2, "Error: switch number must be [1..18] or [153..156]\r\n");
        return;
    }
    static auto ss = whoIs(SwitchServ).asValue();

    ~send(ss, Message{MsgType::Update, sw, dir}, ctl::EmptyMessage);
}

void switchMan() {
    ~ctl::registerAs(SwitchServ);
    SwitchStates ss;
    for (int i = 0; i < NumSwitches; ++i) {
        ss.states[i] = SwitchState::Unknown;
    }
    ~create(ctl::Priority(26), notifMain);
    auto notifier = ctl::INVALID_TID;

    ctl::CircularBuffer<Message, NumSwitches*2> queue;
    ctl::CircularBuffer<ctl::Tid, NUM_TD> waiting;

    auto notifyNotifier = [&](ctl::Tid notif, const Message &msg) {
        ~reply(notif, msg);
        ss[msg.sw] = msg.dir;
        updateGui(msg.sw, msg.dir, ss);
        while (!waiting.empty()) {
            ~reply(waiting.pop(), ss);
        }
    };

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
        case MsgType::Update:
            if (ss[msg.sw] != msg.dir) {
                if (!(notifier == ctl::INVALID_TID)) {
                    notifyNotifier(notifier, msg);
                    notifier = ctl::INVALID_TID;
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
                notifyNotifier(tid, msgToSend);
            }
            break;
        case MsgType::GetData:
            ~reply(tid, ss);
            break;
        case MsgType::WaitChange:
            waiting.push(tid);
            break;
        }
    }
}

SwitchStates getSwitchData() {
    auto switchServTid = whoIs(SwitchServ).asValue();
    SwitchStates ss;
    ~send(switchServTid, Message{MsgType::GetData}, ss);
    return ss;
}

SwitchStates waitSwitchChange() {
    auto switchServTid = whoIs(SwitchServ).asValue();
    SwitchStates ss;
    ~send(switchServTid, Message{MsgType::WaitChange}, ss);
    return ss;
}
