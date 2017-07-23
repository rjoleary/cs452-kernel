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
        Time lastUpdate = 0; // last time of sensor attribution
        Speed speed = 0;
        Velocity velocity = 0;
        Distance stoppingDistance = 0;
        Position position;
        Sensor lastSensor;
        Gasp gasp;
    };

    static void create();

    SafetyServer();

    // Set the speed of a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setTrainSpeed(Train train, Speed speed);

    // Reverse a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error reverseTrain(Train train);

    // Get the state of a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error getTrainState(Train train, TrainState *state);

    // Set GASP for a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setGasp(Train train, const Gasp &gasp);

    // Start calibration for a train.
    // TODO: this should be done in application layer
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error calibrate(Train train, Sensor sensor, Speed speed);
};

struct SafetyState {
    Time lastUpdate; // last time positions were updated
    FixedMap<MAX_CONCURRENT_TRAINS, Train, SafetyServer::TrainState> trains;
    SwitchStates switches;
    struct {
        bool has = false;
        Train train;
        SafetyServer::TrainState state;
    } newTrain;

    // Return the next edge of the node.
    const TrackEdge &nodeEdge(NodeIdx) const;
    void updateTrainStates();
    void updateTrainAtSensor(Train train, Sensor sensor);
};
