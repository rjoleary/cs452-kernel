#pragma once

#include "cache.h"
#include "err.h"
#include "gasp.h"
#include "sensor.h"
#include "switch.h"
#include "train.h"
#include "types.h"

// Maximum number of trains that can be on the track at once.
constexpr auto MAX_CONCURRENT_TRAINS = 8;

// Train red zone - the minimum distance between stopped trains, measured in mm.
constexpr auto TRAIN_RED_ZONE = 100;
// Train yellow zone - the minimum distance between stopped trains before the routing
// layer is notified, measured in mm.
constexpr auto TRAIN_YELLOW_ZONE = 200;
static_assert(TRAIN_RED_ZONE < TRAIN_YELLOW_ZONE, "Red zone must be subset of yellow zone");

// Switch red zone - the distance surrounding a switch for which the switch may
// not be toggled, measured in mm.
constexpr auto SWITCH_RED_ZONE = 200;
// Switch yellow zone - the distance surrounding a switch for which the routing
// layer will be notified of a train's approach.
constexpr auto SWITCH_YELLOW_ZONE = 400;
static_assert(SWITCH_RED_ZONE < SWITCH_YELLOW_ZONE, "Red zone must be subset of yellow zone");

class ModelServer {
    ctl::Tid tid;
public:
    // Representation of a train's state.
    struct TrainState {
        Speed speed;
        Velocity velocity;
        Distance stoppingDistance;
        Position position;
        Gasp gasp;
    };

    static void create();

    ModelServer();

    // Set the speed of a train.
    // Returns:
    //   ctl::Error::Ok: success
    //   ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setTrainSpeed(Train train, Speed speed);

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
};

struct ModelState {
    Time lastUpdate;
    Cache<MAX_CONCURRENT_TRAINS, Train, ModelServer::TrainState> trains;
    SwitchState switches;
    void updateTrainStates();
    void updateTrainAtSensor(Train train, Sensor sensor);
};
