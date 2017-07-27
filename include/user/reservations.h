#pragma once

#include "fixed_map.h"
#include "path_finding.h"
#include "safety.h"
#include "sensor.h"
#include "std.h"
#include "train.h"

// Keeps track of reservations for the safety layer.
class Reservations {
    struct TrainReservation {
        Size length = 0;
        NodeIdx reservations[15];
        bool isReversing = false;
        bool isStopping = false;
        Distance totalDistance = 0;
        NodeIdx reverseNode;
    };

    struct Waitlist {
        Size length = 0;
        Train trains[15];
    } waitlist;

    const SafetyState &safety_;
    TrainServer trainServer_;
    FixedMap<MAX_CONCURRENT_TRAINS, Train, TrainReservation> trainReservations_;

    void flipSwitchesInReservation(Train t, const TrainReservation &r);
    bool checkForReverseInReservation(Train t,
            TrainReservation &r, Distance *out);
    bool checkForStopInReservation(Train t,
            TrainReservation &r, Distance *out);
    bool reserveNode(Train train, NodeIdx node);
    bool reserveForTrain(Train train);
  public:
    Reservations(const SafetyState &safety);
    void printReservations() const;
    // Return true if there is no contention.
    void processUpdate(Train train);
    bool hasReservation(Train, NodeIdx) const;
    void clearStopping(Train);
    NodeIdx clearReversing(Train);
};
