#include <train.h>

#include <bwio.h>
#include <circularbuffer.h>
#include <clock.h>
#include <def.h>
#include <itc.h>
#include <ns.h>
#include <std.h>
#include <task.h>

namespace {
enum class MsgType : char {
    CheckIn,
    LightOn,
    LightOff,
    LightToggle,
    SetSpeed,
    Reverse,
};

struct alignas(4) Message {
    MsgType type;
    Train train;
    char speed;
};

// Message sent to new trains.
struct alignas(4) InitMessage {
    Train train;
};

// Manages a single train.
void trainMain() {
    // Receive train number from the train man.
    InitMessage initMessage;
    ctl::Tid trainMan;
    ~receive(&trainMan, initMessage);
    ~reply(trainMan, ctl::EmptyMessage);

    Train number(initMessage.train);

    // Register as "TrainXX".
    ctl::Name name{"TrainXX"};
    int n = number.underlying();
    name.data[5] = n / 10 % 10 + '0';
    name.data[6] = n % 10 + '0';
    ~registerAs(name);

    // Get clock server.
    ctl::Tid clockServer = whoIs(ctl::names::ClockServer).asValue();

    // Train state
    char speed = 0; // light and speed
    const char LIGHT_MASK = 0x10;
    const char SPEED_MASK = 0x0f;

    // Receive messages from the train man.
    Message msg{MsgType::CheckIn, number};
    for (;;) {
        Message rply;
        ~send(trainMan, msg, rply);

        switch (rply.type) {
            case MsgType::LightOn: {
                speed = speed & LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number.underlying());
                flush(COM1);
                break;
            }

            case MsgType::LightOff: {
                speed = speed & ~LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number.underlying());
                flush(COM1);
                break;
            }

            case MsgType::LightToggle: {
                speed = speed ^ LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number.underlying());
                flush(COM1);
                break;
            }

            case MsgType::SetSpeed: {
                speed = (speed & ~SPEED_MASK) | (rply.speed & SPEED_MASK);
                bwputc(COM1, speed);
                bwputc(COM1, number.underlying());
                flush(COM1);
                break;
            }

            case MsgType::Reverse: {
                if (speed & SPEED_MASK) {
                    // Stop
                    bwputc(COM1, speed & ~SPEED_MASK);
                    bwputc(COM1, number.underlying());
                    flush(COM1);
                    // 200 milliseconds for each train speed
                    ~ctl::delay(clockServer, 20 * (speed & SPEED_MASK));
                }
                // Reverse
                bwputc(COM1, speed | SPEED_MASK);
                bwputc(COM1, number.underlying());
                flush(COM1);
                // Without this delay, sometimes train does not reverse
                ~ctl::delay(clockServer, 10);
                // Set speed
                bwputc(COM1, speed);
                bwputc(COM1, number.underlying());
                flush(COM1);
                break;
            }

            default: {
                ASSERT(false);
            }
        }
    }
}

constexpr ctl::Name TrManName = {"TrMan"};
}

// Manages all the trains.
void trainManMain() {
    ~ctl::registerAs(TrManName);

    // Maps trains to their Tids.
    // Indexing: train #1 is at index 0
    struct Train {
        ctl::Tid tid = ctl::INVALID_TID;
        bool waiting = false;
        ctl::CircularBuffer<Message, 4> messages;
    };
    Train trains[80];

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        Train &train = trains[msg.train.underlying()-1];

        // If this is a train checking in, reply with the queued message.
        if (msg.type == MsgType::CheckIn) {
            if (!train.messages.empty()) {
                ~reply(tid, train.messages.pop());
                train.waiting = false;
            } else {
                train.waiting = true;
            }
        } else {
            // Create new task for the train if one does not exist.
            if (train.tid == ctl::INVALID_TID) {
                train.tid = create(ctl::Priority(26), trainMain).asValue();
                ~send(train.tid, InitMessage{msg.train}, ctl::EmptyMessage);
            }

            // Send the message to the train or queue it.
            if (train.waiting) {
                ~reply(train.tid, msg);
                train.waiting = false;
            } else {
                train.messages.push(msg);
            }

            ~reply(tid, ctl::EmptyMessage);
        }
    }
}

TrainServer::TrainServer()
    : tid(whoIs(TrManName).asValue()) {
}

void TrainServer::stopTrains() {
    bwputc(COM1, 97);
    flush(COM1);
}

void TrainServer::goTrains() {
    bwputc(COM1, 96);
    flush(COM1);
}

void TrainServer::cmdToggleLight(Train train) {
    if (train.underlying() < 1 || 80 < train.underlying()) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    Message msg{MsgType::LightToggle, train};
    ~send(tid, msg, ctl::EmptyMessage);
}

void TrainServer::cmdSetSpeed(Train train, int speed) {
    if (train.underlying() < 1 || 80 < train.underlying()) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    if (speed < 0 || 14 < speed) {
        bwputstr(COM2, "Error: speed must be between 0 and 14 inclusive\r\n");
        return;
    }
    Message msg{MsgType::SetSpeed, train, char(speed)};
    ~send(tid, msg, ctl::EmptyMessage);
}

void TrainServer::cmdReverse(Train train) {
    if (train.underlying() < 1 || 80 < train.underlying()) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    Message msg{MsgType::Reverse};
    ~send(tid, msg, ctl::EmptyMessage);
}