#include "model.h"

#include "itc.h"
#include "ns.h"
#include "task.h"

namespace {
enum class MsgType {
    SetTrainSpeed,
    GetTrainState,
    ListenHazards,
};

struct Message {
    MsgType type;
    Train train;
    Speed speed;
};

struct SetTrainSpeedReply {
    ctl::Error error;
};

struct GetTrainStateReply {
    ctl::Error error;
    Model::TrainState state; // TODO: must copy, a bit inefficient
};

struct ListenHazardsReply {
    ctl::Error error;
    Model::Hazard hazard; // TODO: must copy, a bit inefficient
};

void modelMain() {
    ~ctl::registerAs(ctl::Name{"Model"});

    // TODO
}
} // anonymous namespace

void Model::create() {
    ~ctl::create(ctl::Priority(30), modelMain);
}

Model::Model()
    : tid(ctl::whoIs(ctl::Name{"Model"}).asValue()) {
}

ctl::Error Model::setTrainSpeed(Train train, Speed speed) {
    Message msg;
    msg.type = MsgType::SetTrainSpeed;
    msg.train = train;
    msg.speed = speed;
    SetTrainSpeedReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error Model::getTrainState(Train train, TrainState *state) {
    Message msg;
    msg.type = MsgType::GetTrainState;
    msg.train = train;
    GetTrainStateReply reply;
    ~send(tid, msg, reply);
    *state = reply.state;
    return reply.error;
}

ctl::Error Model::listenHazards(Hazard *hazard) {
    return listenHazards(INVALID_TRAIN, hazard);
}

ctl::Error Model::listenHazards(Train train, Hazard *hazard) {
    Message msg;
    msg.type = MsgType::ListenHazards;
    msg.train = train;
    ListenHazardsReply reply;
    ~send(tid, msg, reply);
    *hazard = reply.hazard;
    return reply.error;
}
