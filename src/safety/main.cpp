#include <safety.h>

#include <attribution.h>
#include <callibration.h>
#include <clock.h>
#include <itc.h>
#include <ns.h>
#include <path_finding.h>
#include <reservations.h>
#include <sensor.h>
#include <task.h>
#include <track.h>

namespace {
enum class MsgType {
    SetTrainSpeed,
    ReverseTrain,
    GetTrainState,
    SetGasp,
    SetSwitch,
    SensorNotify,
    SwitchNotify,
    Calibrate,
    ReverseComplete,
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

struct alignas(4) ErrorReply {
    ctl::Error error = ctl::Error::Ok;
};

struct alignas(4) GetTrainStateReply {
    ctl::Error error = ctl::Error::Ok;
    SafetyServer::TrainState state; // TODO: must copy, a bit inefficient
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

Velocity speedToVelocity(Speed x) {
    return x * VELOCITY_CONSTANT / 2;
}

// Shopkeeper
void safetyMain() {
    ~ctl::registerAs(SafetyServName);
    ~ctl::create(ctl::Priority(24), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);
    TrainServer trainServer;
    CallibrationServer callibration;

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
                SafetyServer::TrainState *ts;
                auto err = state.getTrainStateOrUnattributed(msg.train, &ts);
                if (err != ctl::Error::Ok) {
                    ~reply(tid, ErrorReply{err});
                    break;
                }
                ts->velocity = speedToVelocity(msg.speed);
                ts->speed = msg.speed;
                ts->stoppingDistance = callibration.getStoppingDistance(msg.train, msg.speed);
                ts->stoppedAt = time(clock).asValue();
                // TODO: take safety into account
                //if (newTrain) {
                reservations.clearStopping(msg.train);
                trainServer.setTrainSpeed(msg.train, msg.speed);
                //} else {
                //    reservations.processUpdate(msg.train);
                //}
                ~reply(tid, ErrorReply{});
                break;
            }

            case MsgType::ReverseTrain: {
                SafetyServer::TrainState *ts;
                auto err = state.getTrainStateOrUnattributed(msg.train, &ts);
                if (err != ctl::Error::Ok) {
                    ~reply(tid, ErrorReply{err});
                    break;
                }
                // TODO: safety
                (void)ts;
                reservations.clearStopping(msg.train);
                trainServer.reverseTrain(msg.train);
                ~reply(tid, ErrorReply{});
                break;
            }

            case MsgType::GetTrainState: {
                GetTrainStateReply rply;
                if (!state.trains.has(msg.train)) {
                    rply.error = ctl::Error::BadArg;
                    ~reply(tid, rply);
                    break;
                }
                // TODO: reduce copying
                rply.state = state.trains.get(msg.train);
                ~reply(tid, rply);
                break;
            }

            case MsgType::SetGasp: {
                SafetyServer::TrainState *ts;
                auto err = state.getTrainStateOrUnattributed(msg.train, &ts);
                if (err != ctl::Error::Ok) {
                    ~reply(tid, ErrorReply{err});
                    break;
                }
                reservations.clearStopping(msg.train);
                ts->gasp = msg.gasp;
                ~reply(tid, ErrorReply{});
                break;
            }

            case MsgType::SetSwitch: {
                SafetyServer::TrainState *ts;
                auto err = state.getTrainStateOrUnattributed(msg.train, &ts);
                if (err != ctl::Error::Ok) {
                    ~reply(tid, ErrorReply{err});
                    break;
                }
                ts->gasp.gradient[msg.sw] = msg.state;
                ~reply(tid, ErrorReply{});
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
                    // Attempt to attribute the sensor to a new train.
                    if (state.unattributedTrain != INVALID_TRAIN) {
                        t = state.unattributedTrain;
                        state.unattributedTrain = INVALID_TRAIN;
                        auto &ts = state.trains.get(t);
                        ts = state.unattributedTrainState;
                        ts.lastKnownNode = msg.sensor.value();
                        ts.lastUpdate = time(clock).asValue();
                        ts.position = {msg.sensor.value(), 0};
                    } else {
                        INFOF(45, "\033[31mUnnattributed sensor %d\033[0m\r\n", msg.sensor.value());
                        break;
                    }
                } else {
                    // Attribute a train with an already known position.
                    t = erroror.asValue();
                    state.updateTrainAtSensor(t, msg.sensor);
                }
                INFOF(45, "Sensor %d attributed for %d\r\n",
                        msg.sensor.value(), int(t.underlying()));
                // Reservations stay fixed while the train is reversing.
                reservations.processUpdate(t);
                reservations.printReservations();
                break;
            }

            case MsgType::SwitchNotify: {
                ~reply(tid, ctl::EmptyMessage);
                state.switches[msg.sw] = msg.state;
                break;
            }

            // TODO: move to application layer
            case MsgType::Calibrate: {
                calibration.train = msg.train;
                calibration.sensor = msg.sensor;
                trainServer.setTrainSpeed(msg.train, msg.speed);
                ~reply(tid, ErrorReply{});
                break;
            }

            case MsgType::ReverseComplete: {
                ~reply(tid, ctl::EmptyMessage);
                SafetyServer::TrainState *ts;
                state.getTrainStateOrUnattributed(msg.train, &ts);
                ts->stoppedAt = time(clock).asValue();
                state.trains.get(msg.train).lastKnownNode = reservations.clearReversing(msg.train);
                reservations.processUpdate(msg.train);
                reservations.printReservations();
                break;
            }
        }
    }
}
} // anonymous namespace

ctl::Error SafetyState::getTrainStateOrUnattributed(Train t, SafetyServer::TrainState **ts) {
    // If the train is new
    if (!trains.has(t)) {
        if (unattributedTrain == t) {
            *ts = &unattributedTrainState;
            return ctl::Error::Ok;
        }
        if (unattributedTrain != INVALID_TRAIN) {
            INFOF(44, "Error: cannot attribute train %d and %d at the same time",
                    t, unattributedTrain);
            flush(COM2);
            return ctl::Error::BadArg;
        }
        if (trains.willOverflow(t)) {
            return ctl::Error::NoRes;
        }

        // Set the train's initial state.
        unattributedTrain = t;
        unattributedTrainState.lastUpdate = SafetyServer::TrainState::NEVER_ATTRIBUTED;
        unattributedTrainState.speed = 0;
        unattributedTrainState.velocity = 0;
        unattributedTrainState.stoppingDistance = 0;
        unattributedTrainState.position = INVALID_POSITION;
        unattributedTrainState.gasp.gradient.fill(SwitchState::DontCare); // free running mode
        unattributedTrainState.gasp.end = INVALID_POSITION;
        *ts = &unattributedTrainState;
    } else {
        // The train is not new.
        *ts = &trains.get(t);
    }
    return ctl::Error::Ok;
}

const TrackEdge &SafetyState::nodeEdge(NodeIdx idx) const {
    const auto &tn = Track().nodes[idx];
    if (tn.type == NODE_BRANCH) {
        return tn.edge[switches[tn.num] == SwitchState::Curved];
    }
    return tn.edge[DIR_AHEAD];
}

const TrackEdge &SafetyState::nodeEdge(NodeIdx idx, Train tr) const {
    const auto &tn = Track().nodes[idx];
    const auto &t = trains.get(tr);
    if (tn.type == NODE_BRANCH) {
        if (t.gasp.gradient[tn.num] == SwitchState::DontCare)
            return tn.edge[switches[tn.num] == SwitchState::Curved];
        return tn.edge[t.gasp.gradient[tn.num] == SwitchState::Curved];
    }
    return tn.edge[DIR_AHEAD];
}

void SafetyState::updateTrainStates() {
    /*
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update the position of every train.
    for (auto &ts : trains.values()) {
        ts.position.offset += ts.velocity * (t - lastUpdate);

        // Normalize the node.
        while (ts.position.offset/VELOCITY_CONSTANT > nodeEdge(ts.position.nodeIdx).dist) {
            ts.position.nodeIdx = nodeEdge(ts.position.nodeIdx).dest - Track().nodes;
            ts.position.offset -= nodeEdge(ts.position.nodeIdx).dist * VELOCITY_CONSTANT;
        }
    }
    lastUpdate = t;
    */
}

void SafetyState::updateTrainAtSensor(Train train, Sensor sensor) {
    const static auto clock = whoIs(ctl::names::ClockServer).asValue();
    const auto t = time(clock).asValue();

    // Update velocity based on sensor.
    SafetyServer::TrainState &ts = trains.get(train);
    Distance d = shortestDistance<Graph::VSize>(Track(), ts.lastKnownNode, sensor.value());
    ts.lastKnownNode = sensor.value();
    ts.velocity = (ts.velocity + d * VELOCITY_CONSTANT / (t - ts.lastUpdate)) / 2;
    //ts.stoppingDistance = ctl::max(ctl::min(d*100 / (t - ts.lastUpdate), 650), 250);
    ts.lastUpdate = t;

    // Update the train's current position to be ontop of the sensor.
    ts.position.nodeIdx = sensor.value();
    ts.position.offset = 0;

    INFOF(46, "Velocity: %d\r\n", ts.velocity);
}

void SafetyServer::create() {
    ~ctl::create(ctl::Priority(23), safetyMain);
}

SafetyServer::SafetyServer()
    : tid(ctl::whoIs(SafetyServName).asValue()) {
}

ctl::Error SafetyServer::setTrainSpeed(Train train, Speed speed) {
    Message msg;
    msg.type = MsgType::SetTrainSpeed;
    msg.train = train;
    msg.speed = speed;
    ErrorReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::reverseTrain(Train train) {
    Message msg;
    msg.type = MsgType::ReverseTrain;
    msg.train = train;
    ErrorReply reply;
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
    ErrorReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::setSwitch(Train train, Switch swi, SwitchState state) {
    Message msg;
    msg.type = MsgType::SetSwitch;
    msg.train = train;
    msg.sw = swi;
    msg.state = state;
    ErrorReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

ctl::Error SafetyServer::calibrate(Train train, Sensor sensor, Speed speed) {
    Message msg;
    msg.type = MsgType::Calibrate;
    msg.train = train;
    msg.sensor = sensor;
    msg.speed = speed;
    ErrorReply reply;
    ~send(tid, msg, reply);
    return reply.error;
}

void SafetyServer::reverseComplete(Train train) {
    Message msg;
    msg.type = MsgType::ReverseComplete;
    msg.train = train;
    ~send(tid, msg, ctl::EmptyMessage);
}
