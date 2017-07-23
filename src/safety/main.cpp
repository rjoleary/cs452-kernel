#include <safety.h>

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
    ReverseTrain,
    GetTrainState,
    SetGasp,
    SensorNotify,
    SwitchNotify,
    Calibrate,
};

struct alignas(4) Message {
    MsgType type;
    Train train;
    Speed speed;
    Sensor sensor;
    Switch sw;
    SwitchState state;
    Gasp gasp; // TODO: this is quite large for every message. Possibly use a union?
};

struct alignas(4) SetTrainSpeedReply {
    ctl::Error error = ctl::Error::Ok;
};

struct alignas(4) ReverseTrainReply {
    ctl::Error error = ctl::Error::Ok;
};

struct alignas(4) GetTrainStateReply {
    ctl::Error error = ctl::Error::Ok;
    SafetyServer::TrainState state; // TODO: must copy, a bit inefficient
};

struct alignas(4) SetGaspReply {
    ctl::Error error = ctl::Error::Ok;
};

struct alignas(4) CalibrateReply {
    ctl::Error error = ctl::Error::Ok;
};

constexpr ctl::Name SafetyServName = {"Safety"};

void sensorNotifierMain() {
    auto serv = whoIs(SafetyServName).asValue();
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

void switchNotifierMain() {
    auto serv = whoIs(SafetyServName).asValue();
    Message msg;
    msg.type = MsgType::SwitchNotify;
    auto switches = getSwitchData();
    for (;;) {
        auto newSwitches = waitSwitchChange();
        for (int i = 0; i < NumSwitches; ++i) {
            if (switches.states[i] != newSwitches.states[i]) {
                msg.sw = switches.fromIdx(i);
                msg.state = newSwitches.states[i];
                ~send(serv, msg, ctl::EmptyMessage);
            }
        }
        switches = newSwitches;
    }
}

// Shopkeeper
void modelMain() {
    ~ctl::registerAs(SafetyServName);
    ~ctl::create(ctl::Priority(24), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);
    TrainServer trainServer;

    SafetyState state;
    state.switches = getSwitchData();
    Reservations reservations(state);
    Attribution attribution(state, reservations);

    struct {
        Train train = INVALID_TRAIN;
        Sensor sensor;
    } calibration;

    auto clock = ctl::whoIs(ctl::names::ClockServer).asValue();

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
                SafetyServer::TrainState *ts;
                if (!state.trains.has(msg.train)) {
                    state.newTrain.has = true;
                    state.newTrain.train = msg.train;
                    ts = &state.newTrain.state;
                } else {
                    ts = &state.trains.get(msg.train);
                }
                ts->velocity = msg.speed/2;
                ts->speed = msg.speed;
                ts->stoppingDistance = msg.speed*38;
                // TODO: cut the middleman
                trainServer.setTrainSpeed(msg.train, msg.speed);
                ~reply(tid, rply);
                break;
            }

            case MsgType::ReverseTrain: {
                ReverseTrainReply rply;
                if (state.trains.willOverflow(msg.train)) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                trainServer.reverseTrain(msg.train);
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
                break;
            }

            case MsgType::SensorNotify: {
                ~reply(tid, ctl::EmptyMessage);
                // if calibrating:
                if (calibration.train != INVALID_TRAIN) {
                    if (msg.sensor == calibration.sensor) {
                        trainServer.setTrainSpeed(calibration.train, 0);
                        calibration.train = INVALID_TRAIN;
                    }
                }

                state.updateTrainStates();
                auto erroror = attribution.attribute(msg.sensor);
                Train t;
                if (erroror.isError()) {
                    // If we have a new train, we attribute the sensor to it
                    if (state.newTrain.has) {
                        t = state.newTrain.train;
                    } else {
                        bwprintf(COM2, "\033[45;1HUnnattributed sensor %d\r\n", msg.sensor.value());
                        // TODO: no known attribution
                        break;
                    }
                }
                else {
                    t = erroror.asValue();
                }
                bwprintf(COM2, "\033[45;1HSensor %d attributed for %d\r\n",
                        msg.sensor.value(), int(t.underlying()));
                // If this is a new train
                if (erroror.isError()) {
                    state.newTrain.has = false;
                    state.newTrain.state.lastSensor = msg.sensor;
                    state.newTrain.state.lastUpdate = time(clock).asValue();
                    state.trains.get(state.newTrain.train) = state.newTrain.state;
                }
                else {
                    state.updateTrainAtSensor(t, msg.sensor);
                }
                reservations.processSensor(t, msg.sensor, state.trains.get(t).speed);
                reservations.printReservations();
                break;
            }

            case MsgType::SwitchNotify: {
                ~reply(tid, ctl::EmptyMessage);
                state.switches[msg.sw] = msg.state;
                break;
            }

            case MsgType::Calibrate: {
                CalibrateReply rply;
                if (state.trains.willOverflow(msg.train)) {
                    rply.error = ctl::Error::NoRes;
                    ~reply(tid, rply);
                    break;
                }
                calibration.train = msg.train;
                calibration.sensor = msg.sensor;
                trainServer.setTrainSpeed(msg.train, msg.speed);
                ~reply(tid, rply);
                break;
            }
        }
    }
}
} // anonymous namespace

const TrackEdge &SafetyState::nodeEdge(NodeIdx idx) const {
    const auto &tn = Track().nodes[idx];
    if (tn.type == NODE_BRANCH) {
        return tn.edge[switches[tn.num] == SwitchState::Curved];
    }
    return tn.edge[DIR_AHEAD];
}

void SafetyState::updateTrainStates() {
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update the position of every train.
    for (auto &ts : trains.values()) {
        ts.position.offset += ts.velocity * (t - lastUpdate);

        // Normalize the node.
        while (ts.position.offset > nodeEdge(ts.position.nodeIdx).dist) {
            ts.position.nodeIdx = nodeEdge(ts.position.nodeIdx).dest - Track().nodes;
            ts.position.offset -= nodeEdge(ts.position.nodeIdx).dist;
        }
    }
    lastUpdate = t;
}

void SafetyState::updateTrainAtSensor(Train train, Sensor sensor) {
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update velocity based on sensor.
    SafetyServer::TrainState &ts = trains.get(train);
    Distance d = shortestDistance<Graph::VSize>(Track(), ts.lastSensor.value(), sensor.value());
    ts.lastSensor = sensor;
    ts.velocity = d / (t - ts.lastUpdate);
    //ts.stoppingDistance = ctl::max(ctl::min(d*100 / (t - ts.lastUpdate), 650), 250);
    ts.lastUpdate = t;

    // Update the train's current position to be ontop of the sensor.
    ts.position.nodeIdx = sensor.value();
    ts.position.offset = 0;

    bwprintf(COM2, "Velocity: %d\r\n", ts.velocity);
}

void SafetyServer::create() {
    ~ctl::create(ctl::Priority(23), modelMain);
}

SafetyServer::SafetyServer()
    : tid(ctl::whoIs(SafetyServName).asValue()) {
}

ctl::Error SafetyServer::setTrainSpeed(Train train, Speed speed) {
    Message msg;
    msg.type = MsgType::SetTrainSpeed;
    msg.train = train;
    msg.speed = speed;
    SetTrainSpeedReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::reverseTrain(Train train) {
    Message msg;
    msg.type = MsgType::ReverseTrain;
    msg.train = train;
    ReverseTrainReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::getTrainState(Train train, TrainState *state) {
    Message msg;
    msg.type = MsgType::GetTrainState;
    msg.train = train;
    GetTrainStateReply reply;
    ~send(tid, msg, reply);
    *state = reply.state;
    return reply.error;
}

ctl::Error SafetyServer::setGasp(Train train, const Gasp &gasp) {
    Message msg;
    msg.type = MsgType::SetGasp;
    msg.train = train;
    msg.gasp = gasp;
    SetGaspReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::calibrate(Train train, Sensor sensor, Speed speed) {
    Message msg;
    msg.type = MsgType::Calibrate;
    msg.train = train;
    msg.sensor = sensor;
    msg.speed = speed;
    CalibrateReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}
