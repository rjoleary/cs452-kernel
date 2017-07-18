#include <model.h>

#include <attribution.h>
#include <itc.h>
#include <ns.h>
#include <reservations.h>
#include <sensor.h>
#include <task.h>
#include <track.h>
#include <clock.h>
#include <path_finding.h>

namespace {
enum class MsgType {
    SetTrainSpeed,
    GetTrainState,
    SetGasp,
    SensorNotify,
};

struct alignas(4) Message {
    MsgType type;
    Train train;
    Speed speed;
    Sensor sensor;
    Gasp gasp; // TODO: this is quite large for every message
};

struct alignas(4) SetTrainSpeedReply {
    ctl::Error error = ctl::Error::Ok;
};

struct alignas(4) GetTrainStateReply {
    ctl::Error error = ctl::Error::Ok;
    ModelServer::TrainState state; // TODO: must copy, a bit inefficient
};

struct alignas(4) SetGaspReply {
    ctl::Error error = ctl::Error::Ok;
};

constexpr ctl::Name ModelServName = {"Model"};

void sensorNotifierMain() {
    auto serv = whoIs(ModelServName).asValue();
    Message msg;
    msg.type = MsgType::SensorNotify;
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
    ~ctl::registerAs(ModelServName);
    ~ctl::create(ctl::Priority(24), sensorNotifierMain);
    TrainServer trainServer;

    ModelState state;
    Reservations reservations(state);
    Attribution attribution(state, reservations);

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
                    break;
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
                    break;
                }
                // TODO: reduce copying
                rply.state = state.trains.get(idx);
                ~reply(tid, rply);
                break;
            }

            case MsgType::SetGasp: {
                SetGaspReply rply;
                auto idx = state.trains.getIdx(msg.train);
                if (idx == (Size)-1) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                state.trains.get(idx).gasp = msg.gasp;
                ~reply(tid, rply);
            }

            case MsgType::SensorNotify: {
                ~reply(tid, ctl::EmptyMessage);
                state.updateTrainStates();
                auto erroror = attribution.attribute(msg.sensor);
                if (erroror.isError()) {
                    // TODO: no known attribution
                    break;
                }
                Train t = erroror.asValue();
                state.updateTrainAtSensor(t, msg.sensor);
                const bool TRAINS_WILL_COLLIDE = false;
                if (reservations.sensorTriggered(t, msg.sensor) == TRAINS_WILL_COLLIDE) {
                    trainServer.cmdSetSpeed(t, 0);
                }
                break;
            }
        }
    }
}
} // anonymous namespace

const TrackEdge &ModelState::nodeEdge(NodeIdx idx) const {
    const auto &tn = Track.nodes[idx];
    if (tn.type == NODE_BRANCH) {
        return tn.edge[switches[tn.num] == 'C'];
    }
    return tn.edge[DIR_AHEAD];
}

void ModelState::updateTrainStates() {
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update the position of every train.
    for (Size i = 0; i < trains.size(); i++) {
        auto &ts = trains.get(i);
        ts.position.offset += ts.velocity * (t - lastUpdate);

        // Normalize the node.
        while (ts.position.offset > nodeEdge(ts.position.nodeIdx).dist) {
            ts.position.nodeIdx = nodeEdge(ts.position.nodeIdx).dest - Track.nodes;
            ts.position.offset -= nodeEdge(ts.position.nodeIdx).dist;
        }
    }
    lastUpdate = t;
}

void ModelState::updateTrainAtSensor(Train train, Sensor sensor) {
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update velocity based on sensor.
    ModelServer::TrainState &ts = trains.get(train);
    Distance d = shortestDistance<Graph::VSize>(Track, ts.lastSensor.value(), sensor.value());
    ts.velocity = d / (t - ts.lastUpdate);
    ts.lastUpdate = t;

    // TODO: update stopping distance as a function of velocity
    ts.stoppingDistance = 500;
}

void ModelServer::create() {
    ~ctl::create(ctl::Priority(23), modelMain);
}

ModelServer::ModelServer()
    : tid(ctl::whoIs(ModelServName).asValue()) {
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

ctl::Error ModelServer::setGasp(Train train, const Gasp &gasp) {
    Message msg;
    msg.type = MsgType::SetGasp;
    msg.train = train;
    msg.gasp = gasp;
    SetGaspReply reply;
    ~send(tid, msg, ctl::EmptyMessage);
    return reply.error;
}
