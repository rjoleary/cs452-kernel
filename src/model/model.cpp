#include <model.h>

#include <itc.h>
#include <ns.h>
#include <task.h>
#include <train.h>
#include <cache.h>

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
    ctl::Error error = ctl::Error::Ok;
};

struct GetTrainStateReply {
    ctl::Error error = ctl::Error::Ok;
    Model::TrainState state; // TODO: must copy, a bit inefficient
};

struct ListenHazardsReply {
    ctl::Error error = ctl::Error::Ok;
    Model::Hazard hazard; // TODO: must copy, a bit inefficient
};

void modelMain() {
    ~ctl::registerAs(ctl::Name{"Model"});
    TrainServer trainServer;

    Cache<MAX_CONCURRENT_TRAINS, Train, Model::TrainState> trainStates;

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
            case MsgType::SetTrainSpeed: {
                SetTrainSpeedReply rply;
                auto idx = trainStates.getIdx(msg.train);
                if (idx == (ctl::size_t)-1) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                }
                // TODO: cut the middleman
                trainStates.get(idx).speed = msg.speed;
                trainServer.cmdSetSpeed(msg.train, msg.speed);
                ~reply(tid, rply);
                break;
            }

            case MsgType::GetTrainState: {
                GetTrainStateReply rply;
                auto idx = trainStates.getIdx(msg.train);
                if (idx == (ctl::size_t)-1) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                }
                // TODO: reduce copying
                rply.state = trainStates.get(idx);
                ~reply(tid, rply);
                break;
            }

            case MsgType::ListenHazards: {
                break;
            }
        }
    }
}
} // anonymous namespace

void Model::create() {
    ~ctl::create(ctl::Priority(25), modelMain);
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
