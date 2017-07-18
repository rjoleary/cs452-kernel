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
    UpdateRoute,
};

struct Message {
    MsgType type;
    // Expand to union when new types added
    Train train;
    Position end;
};

} // unnamed namespace

// New routes are created with low priority.
void routeMain() {
    ~ctl::registerAs(RouteServName);
    ModelServer modelServer;
    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::UpdateRoute: {
                ctl::reply(tid, ctl::EmptyMessage);
                auto ss = dijkstra(Graph{Track().nodes}, msg.end.nodeIdx);
                Gasp gasp;
                gasp.gradient = ss;
                gasp.end = msg.end;
                modelServer.setGasp(msg.train, gasp);
            }
        }
    }
}

void updateRoute(Train train, Position end) {
    auto server = ctl::whoIs(RouteServName).asValue();
    ctl::send(server, Message{MsgType::UpdateRoute, train, end}, ctl::EmptyMessage);
}
