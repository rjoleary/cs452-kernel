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
    char train, speed;
};

// Manages a single train.
void trainMain() {
    // Receive train number from the train man.
    unsigned number;
    ctl::Tid trainMan;
    ~receive(&trainMan, number);
    ~reply(trainMan, ctl::EmptyMessage);

    // Register as "TrainXX".
    ctl::Name name{"TrainXX"};
    name.data[5] = number / 10 % 10 + '0';
    name.data[6] = number % 10 + '0';
    ~registerAs(name);

    // Get clock server.
    ctl::Tid clockServer = whoIs(ctl::names::ClockServer).asValue();

    // Train state
    char speed = 0; // light and speed
    const char LIGHT_MASK = 0x10;
    const char SPEED_MASK = 0x0f;

    // Receive messages from the train man.
    Message msg{MsgType::CheckIn, char(number)};
    for (;;) {
        Message rply;
        ~send(trainMan, msg, rply);

        switch (rply.type) {
            case MsgType::LightOn: {
                speed = speed & LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number);
                flush(COM1);
                break;
            }

            case MsgType::LightOff: {
                speed = speed & ~LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number);
                flush(COM1);
                break;
            }

            case MsgType::LightToggle: {
                speed = speed ^ LIGHT_MASK;
                bwputc(COM1, speed);
                bwputc(COM1, number);
                flush(COM1);
                break;
            }

            case MsgType::SetSpeed: {
                speed = (speed & ~SPEED_MASK) | (rply.speed & SPEED_MASK);
                bwputc(COM1, speed);
                bwputc(COM1, number);
                flush(COM1);
                break;
            }

            case MsgType::Reverse: {
                if (speed & SPEED_MASK) {
                    // Stop
                    bwputc(COM1, speed & ~SPEED_MASK);
                    bwputc(COM1, number);
                    flush(COM1);
                    // 200 milliseconds for each train speed
                    ~ctl::delay(clockServer, 20 * (speed & SPEED_MASK));
                }
                // Reverse
                bwputc(COM1, speed | SPEED_MASK);
                bwputc(COM1, number);
                flush(COM1);
                // With this delay, the reverse is sometimes dropped.
                ~ctl::delay(clockServer, 10);
                // Set speed
                bwputc(COM1, speed);
                bwputc(COM1, number);
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
        Train &train = trains[msg.train-1];

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
                train.tid = create(ctl::Priority(20), trainMain).asValue();
                ~send(train.tid, unsigned(msg.train), ctl::EmptyMessage);
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

void stopTrains() {
    bwputc(COM1, 97);
    flush(COM1);
}

void goTrains() {
    bwputc(COM1, 96);
    flush(COM1);
}

void cmdToggleLight(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    Message msg{MsgType::LightToggle, char(train)};
    ~send(whoIs(TrManName).asValue(), msg, ctl::EmptyMessage);
}

void cmdSetSpeed(int train, int speed) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    if (speed < 0 || 14 < speed) {
        bwputstr(COM2, "Error: speed must be between 0 and 14 inclusive\r\n");
        return;
    }
    Message msg{MsgType::SetSpeed, char(train), char(speed)};
    ~send(whoIs(TrManName).asValue(), msg, ctl::EmptyMessage);
}

void cmdReverse(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    Message msg{MsgType::Reverse, char(train)};
    ~send(whoIs(TrManName).asValue(), msg, ctl::EmptyMessage);
}
