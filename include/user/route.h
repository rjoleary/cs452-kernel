#pragma once

#include <task.h>
#include <train.h>

class RouteServer {
    ctl::Tid tid_;
public:
    RouteServer();
    static void create();
    void update(Train train, Speed speed, Position end);
};
