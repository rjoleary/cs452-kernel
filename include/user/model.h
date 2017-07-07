#pragma once

#include "err.h"
#include "switch.h"
#include "train.h"
#include "types.h"

// Maximum number of trains that can be on the track at once.
constexpr auto MAX_CONCURRENT_TRAINS = 8;

// Red zone - the minimum distance between stopped trains, measured in mm.
constexpr auto RED_ZONE = 100;
// Yellow zone - the minimum distance between stopped trains before the routing
// layer is notified, measured in mm.
constexpr auto YELLOW_ZONE = 200;
static_assert(RED_ZONE < YELLOW_ZONE, "Red zone must be subset of yellow zone");

class Model {
    ctl::Tid tid;
public:
    enum class HazardType {
        // The train is approaching a switch.
        ApproachSwitch,

        // The switch is stuck in an unwanted state.
        BrokenSwitch,

        // A train entered the yellow zone. The route layer is expected to
        // reduce speed or reroute the train to avoid collision.
        YellowZone,

        // A train was about to enter the red zone and the model layer has
        // taken protective action to avoid a collision.
        RedZone,
    };

    struct Hazard {
        HazardType type;
        Switch swi;
        Train train;
    };

    // Representation of a position offset from a switch.
    struct Position {
        Switch swi;
        int offset; // mm
    };

    // Representation of a train's state.
    struct TrainState {
        int time; // ticks
        Position position;
        Speed speed;
        int velocity; // mm/tick
    };

    static void create();

    Model();
    //void getSwitchState(Switch swi, );

    // Set the speed of a train.
    // Returns:
    //  ctl::Error::Ok: success
    //  ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error setTrainSpeed(Train train, Speed speed);

    // Get the state of a train.
    // Returns:
    //  ctl::Error::Ok: success
    //  ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error getTrainState(Train train, TrainState *state);

    // Block until the next hazard.
    // Returns:
    //  ctl::Error::Ok: success
    //  ctl::Error::NoRes: more than MAX_CONCURRENT_TRAINS
    ctl::Error listenHazards(Hazard *hazard);
    ctl::Error listenHazards(Train train, Hazard *hazard);
};
