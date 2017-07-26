#include <callibration.h>

#include <itc.h>
#include <ns.h>

namespace {

constexpr ctl::Name CallibrationServName = {"Calli"};

enum class MsgType {
    GetStoppingDistance,
    SetStoppingDistance,
};

struct Message {
    MsgType type;
    Train train;
    Speed speed;
    Distance distance;
};

struct Reply {
    Distance distance;
};

Distance speedToStoppingDistance(Train t, Speed s) {
    switch (t.underlying()) {
        default:
        case 24:
            // TODO: update
            return s <= 6 ? 417 :
                   s <= 8 ? 569 :
                   s <= 10 ? 731 :
                   s <= 12 ? 874 : 569;
        case 63:
            return s <= 6 ? 417 :
                   s <= 8 ? 569 :
                   s <= 10 ? 731 :
                   s <= 12 ? 874 : 569;
        case 71:
            // TODO: update
            return s <= 6 ? 417 :
                   s <= 8 ? 569 :
                   s <= 10 ? 731 :
                   s <= 12 ? 874 : 569;
        case 74:
            // TODO: update
            return s <= 6 ? 417 :
                   s <= 8 ? 569 :
                   s <= 10 ? 731 :
                   s <= 12 ? 874 : 569;
    }
}

void callibrationMain() {
    ~ctl::registerAs(CallibrationServName);

    Distance stoppingDistances[81][15];

    // Clear to -1.
    for (int i = 0; i < 81; i++) {
        for (int j = 0; j < 15; j++) {
            stoppingDistances[i][j] = -1;
        }
    }

    for (;;) {
        ctl::Tid tid;
        Message msg;

        ~ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::GetStoppingDistance: {
                Reply rply;
                rply.distance = stoppingDistances[int(msg.train.underlying())][msg.speed] = msg.distance;
                if (rply.distance == -1) {
                    rply.distance = speedToStoppingDistance(msg.train, msg.speed);
                }
                ~ctl::reply(tid, rply);
                break;
            }

            case MsgType::SetStoppingDistance: {
                stoppingDistances[int(msg.train.underlying())][int(msg.speed)] = msg.distance;
                ~ctl::reply(tid, ctl::EmptyMessage);
                break;
            }
        }
    }
}
} // anonymous namespace

CallibrationServer::CallibrationServer()
    : tid_(ctl::whoIs(CallibrationServName).asValue()) {
}

void CallibrationServer::create() {
    ~ctl::create(ctl::Priority(23), callibrationMain);
}

Distance CallibrationServer::getStoppingDistance(Train train, Speed speed) {
    Message msg;
    msg.type = MsgType::GetStoppingDistance;
    msg.train = train;
    msg.speed = speed;
    Reply rply;
    ~ctl::send(tid_, msg, rply);
    return rply.distance;
}

void CallibrationServer::setStoppingDistance(Train train, Speed speed, Distance distance) {
    Message msg;
    msg.type = MsgType::SetStoppingDistance;
    msg.train = train;
    msg.speed = speed;
    msg.distance = distance;
    ~ctl::send(tid_, msg, ctl::EmptyMessage);
}
