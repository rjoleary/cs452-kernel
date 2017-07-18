#include <itc.h>
#include <def.h>
#include <ns.h>
#include <model.h>
#include <sensor.h>
#include <path_finding.h>
#include <train.h>
#include <track_node.h>
#include <track_data_new.h>
#include <track.h>

namespace {

constexpr auto MAX_PATH = 30;
constexpr ctl::Name RouteServName = {"Route"};

enum class MsgType {
    NewRoute
};

struct Message {
    MsgType type;
    // Expand to union when new types added
    Train train;
    Sensor end;
};

struct RouteReply {
    SwitchState ss;
};

} // unnamed namespace

void routeMain() {
    ~ctl::registerAs(RouteServName);
    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NewRoute: {
                auto ss = dijkstra(Graph{Track.nodes},
                        msg.end.value());
                ctl::reply(tid, RouteReply{ss});
            }
        }
    }
}

SwitchState getRoute(Train train, Sensor end) {
    auto server = ctl::whoIs(RouteServName).asValue();
    RouteReply rr;
    ctl::send(server, Message{MsgType::NewRoute, train, end}, rr);
    return rr.ss;
}
