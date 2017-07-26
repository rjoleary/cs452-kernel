#pragma once

#include <train.h>
#include <types.h>

class CallibrationServer {
    ctl::Tid tid_;
public:
    CallibrationServer();
    static void create();
    Distance getStoppingDistance(Train, Speed);
    void setStoppingDistance(Train, Speed, Distance);
};
