#include <model.h>

#include <attribution.h>
#include <itc.h>
#include <ns.h>
#include <sensor.h>
#include <task.h>
#include <reservations.h>

namespace {
// TODO: move to some standard header
template <typename T>
const T &min(const T &a, const T &b) {
    return a <= b ? a : b;
}

enum class MsgType {
    SetTrainSpeed,
    GetTrainState,
    SensorNotify,
};

struct Message {
    MsgType type;
    Train train;
    Speed speed;
    Sensor sensor;
};

struct SetTrainSpeedReply {
    ctl::Error error = ctl::Error::Ok;
};

struct GetTrainStateReply {
    ctl::Error error = ctl::Error::Ok;
    ModelServer::TrainState state; // TODO: must copy, a bit inefficient
};

void sensorNotifierMain() {
    auto serv = whoIs(ctl::Name{"Model"}).asValue();
    Message msg = {MsgType::SensorNotify};
    for (;;) {
        auto sens = waitTrigger();
        for (Size i = 0; i < NUM_SENSORS; i++) {
            auto s = Sensor::fromInt(i).asValue();
            if (sens(s)) {
                msg.sensor = s;
                ~send(serv, msg, ctl::EmptyMessage);
            }
        }
    }
}

// Shopkeeper
void modelMain() {
    ~ctl::registerAs(ctl::Name{"Model"});
    ~ctl::create(ctl::Priority(26), sensorNotifierMain);
    TrainServer trainServer;

    ModelState state;
    Reservations reservations(state);
    Attribution attribution(state);

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
            case MsgType::SetTrainSpeed: {
                SetTrainSpeedReply rply;
                auto idx = state.trains.getIdx(msg.train);
                if (idx == (Size)-1) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                }
                // TODO: cut the middleman
                state.trains.get(idx).speed = msg.speed;
                trainServer.cmdSetSpeed(msg.train, msg.speed);
                ~reply(tid, rply);
                break;
            }

            case MsgType::GetTrainState: {
                GetTrainStateReply rply;
                auto idx = state.trains.getIdx(msg.train);
                if (idx == (Size)-1) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                }
                // TODO: reduce copying
                rply.state = state.trains.get(idx);
                ~reply(tid, rply);
                break;
            }

            case MsgType::SensorNotify: {
                ~reply(tid, EmptyMessage);
                state.updateTrainStates();
                Train t = attribution.attribute(msg.sensor);
                state.updateTrainAtSensor(t, msg.sensor);
                reservations.sensorTriggered(t, msg.sensor);
                break;
            }
        }
    }
}
} // anonymous namespace

void ModelState::updateTrainStates() {
    // TODO
}

void ModelState::updateTrainAtSensor(Train train, Sensor sensor) {
    // TODO
    (void) train;
    (void) sensor;
}

void ModelServer::create() {
    ~ctl::create(ctl::Priority(25), modelMain);
}

ModelServer::ModelServer()
    : tid(ctl::whoIs(ctl::Name{"Model"}).asValue()) {
}

ctl::Error ModelServer::setTrainSpeed(Train train, Speed speed) {
    Message msg;
    msg.type = MsgType::SetTrainSpeed;
    msg.train = train;
    msg.speed = speed;
    SetTrainSpeedReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error ModelServer::getTrainState(Train train, TrainState *state) {
    Message msg;
    msg.type = MsgType::GetTrainState;
    msg.train = train;
    GetTrainStateReply reply;
    ~send(tid, msg, reply);
    *state = reply.state;
    return reply.error;
}
