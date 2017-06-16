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
// TODO: data framing
void trainMain() {
    // Receive train number from the train man.
    unsigned number;
    ctl::Tid trainMan;
    ASSERT(receive(&trainMan, number) == sizeof(number));
    ASSERT(reply(trainMan, ctl::EmptyMessage) == 0);

    // Register as "TrainXX".
    ctl::Name name{"TrainXX"};
    name.data[5] = number / 10 % 10 + '0';
    name.data[6] = number % 10 + '0';
    ASSERT(registerAs(name) == 0);

    // Get clock server.
    ctl::Tid clockServer = whoIs(ctl::names::ClockServer);
    ASSERT(clockServer.underlying() >= 0);

    // Train state
    char speed = 0; // light and speed
    const char LIGHT_MASK = 0x10;
    const char SPEED_MASK = 0x0f;

    // Receive messages from the train man.
    Message msg{MsgType::CheckIn, char(number)};
    for (;;) {
        Message rply;
        ASSERT(send(trainMan, msg, rply) == sizeof(rply));

        switch (rply.type) {
            case MsgType::LightOn: {
                speed = speed & LIGHT_MASK;
                break;
            }

            case MsgType::LightOff: {
                speed = speed & ~LIGHT_MASK;
                break;
            }

            case MsgType::LightToggle: {
                speed = speed ^ LIGHT_MASK;
                break;
            }

            case MsgType::SetSpeed: {
                speed = (speed & ~SPEED_MASK) | (rply.speed & SPEED_MASK);
                bwputc(COM1, speed);
                bwputc(COM1, number);
                break;
            }

            case MsgType::Reverse: {
                // Stop
                bwputc(COM1, speed & ~SPEED_MASK);
                bwputc(COM1, number);
                // 200 milliseconds for each train speed
                ctl::delay(clockServer, 20);
                // Reverse
                bwputc(COM1, speed | SPEED_MASK);
                bwputc(COM1, number);
                // Set speed
                bwputc(COM1, speed);
                bwputc(COM1, number);
                break;
            }
            
            default: {
                ASSERT(false);
            }
        }
    }
}
}

// Manages all the trains.
void trainManMain() {
    ASSERT(ctl::registerAs(ctl::Name{"TrMan"}) == 0);

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
        ASSERT(receive(&tid, msg) == sizeof(msg));
        Train &train = trains[msg.train-1];

        // If this is a train checking in, reply with the queued message.
        if (msg.type == MsgType::CheckIn) {
            if (!train.messages.empty()) {
                ASSERT(reply(tid, train.messages.pop()) == 0);
                train.waiting = false;
            } else {
                train.waiting = true;
            }
        } else {
            // Create new task for the train if one does not exist.
            if (train.tid == ctl::INVALID_TID) {
                train.tid = ctl::Tid(create(ctl::Priority(20), trainMain));
                ASSERT(train.tid.underlying() >= 0);
                ASSERT(send(train.tid, unsigned(msg.train), ctl::EmptyMessage) == 0);
            }

            // Send the message to the train or queue it.
            if (train.waiting) {
                ASSERT(reply(train.tid, msg) == 0);
                train.waiting = false;
            } else {
                train.messages.push(msg);
            }

            ASSERT(reply(tid, ctl::EmptyMessage) == 0);
        }
    }
}

void stopTrains() {
    bwputc(COM1, 97);
}

void goTrains() {
    bwputc(COM1, 96);
}

void cmdToggleLight(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    Message msg{MsgType::LightToggle, char(train)};
    ASSERT(send(whoIs(ctl::Name{"TrMan"}), msg, ctl::EmptyMessage) == 0);
}

void cmdSetSpeed(int train, int speed) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    if (speed < 0 || 14 < speed) {
        bwputstr(COM2, "Error: speed must be between 0 and 15 inclusive\r\n");
        return;
    }
    Message msg{MsgType::SetSpeed, char(train), char(speed)};
    ASSERT(send(whoIs(ctl::Name{"TrMan"}), msg, ctl::EmptyMessage) == 0);
}

void cmdReverse(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    Message msg{MsgType::Reverse, char(train)};
    ASSERT(send(whoIs(ctl::Name{"TrMan"}), msg, ctl::EmptyMessage) == 0);
}
