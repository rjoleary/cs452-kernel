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
                if (state.trains.willOverflow(msg.train)) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                auto &ts = state.trains.get(msg.train);
                ts.speed = msg.speed;
                // TODO: cut the middleman
                trainServer.cmdSetSpeed(msg.train, msg.speed);
                ~reply(tid, rply);
                break;
            }

            case MsgType::GetTrainState: {
                GetTrainStateReply rply;
                if (state.trains.willOverflow(msg.train)) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                // TODO: reduce copying
                rply.state = state.trains.get(msg.train);
                ~reply(tid, rply);
                break;
            }

            case MsgType::SetGasp: {
                SetGaspReply rply;
                if (state.trains.willOverflow(msg.train)) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                auto &ts = state.trains.get(msg.train);
                ts.gasp = msg.gasp;
                ~reply(tid, rply);
            }

            case MsgType::SensorNotify: {
                ~reply(tid, ctl::EmptyMessage);
                state.updateTrainStates();
                auto erroror = attribution.attribute(msg.sensor);
                Train t;
                if (erroror.isError()) {
                    for (Size i = 0; i < state.trains.size(); ++i) {
                        if (state.trains.get(i).position == INVALID_POSITION) {
                            t = state.trains.getKey(i);
                            goto breakout;
                        }
                    }
                    bwprintf(COM2, "Unnattributed sensor %d\r\n", msg.sensor.value());
                    // TODO: no known attribution
                    break;
                }
                else {
                    t = erroror.asValue();
                }
breakout:
                bwprintf(COM2, "Sensor %d attributed for %d\r\n", msg.sensor.value(), int(t.underlying()));
                state.updateTrainAtSensor(t, msg.sensor);
                const bool TRAINS_WILL_COLLIDE = false;
                if (reservations.sensorTriggered(t, msg.sensor) == TRAINS_WILL_COLLIDE) {
                    bwprintf(COM2, "SensorTriggered\r\n");
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
    for (auto &ts : trains.values()) {
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
