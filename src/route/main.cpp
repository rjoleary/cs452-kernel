#include <route.h>

#include <def.h>
#include <itc.h>
#include <ns.h>
#include <path_finding.h>
#include <safety.h>
#include <sensor.h>
#include <track.h>
#include <track_data_new.h>
#include <track_node.h>
#include <train.h>

namespace {

constexpr ctl::Name RouteServName = {"Route"};

enum class MsgType {
    UpdateRoute,
};

struct Message {
    MsgType type;
    Train train;
    Speed speed;
    Position end;
};

// New routes are created with low priority.
void routeMain() {
    ~ctl::registerAs(RouteServName);
    SafetyServer safetyServer;
    for (;;) {
        ctl::Tid tid;
        Message msg;

        ~ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::UpdateRoute: {
                ~ctl::reply(tid, ctl::EmptyMessage);
                auto ss = dijkstra(Graph{Track().nodes}, msg.end.nodeIdx);
                Gasp gasp;
                gasp.gradient = ss;
                gasp.end = msg.end;
                // TODO: should return the error and not assert
                ASSERT(safetyServer.setGasp(msg.train, gasp) == ctl::Error::Ok);
                ASSERT(safetyServer.setTrainSpeed(msg.train, msg.speed) == ctl::Error::Ok);
            }
        }
    }
}
} // anonymous namespace

RouteServer::RouteServer()
    : tid_(ctl::whoIs(RouteServName).asValue()) {
}

void RouteServer::create() {
    ~ctl::create(ctl::Priority(22), routeMain);
}

void RouteServer::update(Train train, Speed speed, Position end) {
    ~ctl::send(tid_, Message{MsgType::UpdateRoute, train, speed, end}, ctl::EmptyMessage);
}
