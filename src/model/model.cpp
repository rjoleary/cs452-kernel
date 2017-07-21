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
    SwitchNotify,
    Calibrate,
};

struct alignas(4) Message {
    MsgType type;
    Train train;
    Speed speed;
    Sensor sensor;
    char sw;
    char state;
    Gasp gasp; // TODO: this is quite large for every message. Possibly use a union?
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

void switchNotifierMain() {
    auto serv = whoIs(ModelServName).asValue();
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
    ~ctl::registerAs(ModelServName);
    ~ctl::create(ctl::Priority(24), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);
    TrainServer trainServer;

    ModelState state;
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
                ModelServer::TrainState *ts;
                if (!state.trains.has(msg.train)) {
                    state.newTrain.has = true;
                    state.newTrain.train = msg.train;
                    ts = &state.newTrain.state;
                }
                else {
                    ts = &state.trains.get(msg.train);
                }
                ts->velocity = msg.speed/2;
                ts->speed = msg.speed;
                ts->stoppingDistance = msg.speed*38;
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
                // if calibrating:
                if (calibration.train != INVALID_TRAIN) {
                    if (msg.sensor == calibration.sensor) {
                        trainServer.cmdToggleLight(calibration.train);
                        trainServer.cmdSetSpeed(calibration.train, 0);
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
                calibration.train = msg.train;
                calibration.sensor = msg.sensor;
                trainServer.cmdToggleLight(msg.train);
                trainServer.cmdSetSpeed(msg.train, msg.speed);
                ~reply(tid, ctl::EmptyMessage);
                break;
            }
        }
    }
}
} // anonymous namespace

const TrackEdge &ModelState::nodeEdge(NodeIdx idx) const {
    const auto &tn = Track().nodes[idx];
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
            ts.position.nodeIdx = nodeEdge(ts.position.nodeIdx).dest - Track().nodes;
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
    ~send(tid, msg, reply);
    return reply.error;
}

void ModelServer::calibrate(Train train, Sensor sensor, Speed speed) {
    Message msg;
    msg.type = MsgType::Calibrate;
    msg.train = train;
    msg.sensor = sensor;
    msg.speed = speed;
    ~send(tid, msg, ctl::EmptyMessage);
}
