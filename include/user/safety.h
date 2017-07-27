#pragma once

#include "err.h"
#include "fixed_map.h"
#include "gasp.h"
#include "sensor.h"
#include "switch.h"
#include "track_node.h"
#include "train.h"
#include "types.h"

// Maximum number of trains that can be on the track at once.
constexpr auto MAX_CONCURRENT_TRAINS = 8;

class SafetyServer {
    ctl::Tid tid;
public:
    // Representation of a train's state.
    struct TrainState {
        static constexpr Time NEVER_ATTRIBUTED = 0;
        Time lastUpdate;
        Speed speed;
        Velocity velocity;
        Distance stoppingDistance;
        Position position;
        NodeIdx lastKnownNode;
        Gasp gasp;
        Time stoppedAt = 0;
    };

    static void create();

    SafetyServer();

    // Set the speed of a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::BadArg: multiple trains cannot be unattributed at once
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setTrainSpeed(Train train, Speed speed);

    // Reverse a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::BadArg: multiple trains cannot be unattributed at once
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error reverseTrain(Train train);

    // Get the state of a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::BadArg: the train does not exist
    ctl::Error getTrainState(Train train, TrainState *state);

    // Set GASP for a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::BadArg: multiple trains cannot be unattributed at once
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setGasp(Train train, const Gasp &gasp);

    // Set switch state with respect to a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::BadArg: multiple trains cannot be unattributed at once
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setSwitch(Train, Switch, SwitchState);

    // Start calibration for a train.
    // TODO: this should be done in application layer
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error calibrate(Train train, Sensor sensor, Speed speed);

    void reverseComplete(Train train);
};

struct SafetyState {
    Time lastUpdate; // last time positions were updated
    FixedMap<MAX_CONCURRENT_TRAINS, Train, SafetyServer::TrainState> trains;
    SwitchStates switches;
    // The next unattributed sensors gets attributed to this train.
    Train unattributedTrain = INVALID_TRAIN;
    SafetyServer::TrainState unattributedTrainState;

    // Return a pointer to the train state or to the unattributed train.
    ctl::Error getTrainStateOrUnattributed(Train, SafetyServer::TrainState**);
    // Return the next edge of the node.
    const TrackEdge &nodeEdge(NodeIdx) const;
    const TrackEdge &nodeEdge(NodeIdx, Train) const;
    void updateTrainStates();
    void updateTrainAtSensor(Train train, Sensor sensor);
};
