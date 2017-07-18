#include <itc.h>
#include <def.h>
#include <model.h>
#include <sensor.h>
#include <path_finding.h>
#include <train.h>
#include <track_node.h>
#include <track_data_new.h>
#include <track.h>

namespace {

constexpr auto MAX_PATH = 30;

enum class MsgType {
    NewRoute
};

struct Message {
    MsgType type;
    // Expand to union when new types added
    Train train;
    Sensor start, end;
};

struct RouteReply {
    SwitchState ss;
    int length;
};

} // unnamed namespace

void routeMain() {
    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NewRoute: {
                /*auto ss = */dijkstra(Graph{Track.nodes},
                        msg.end.value());
            }
        }
    }
}

// Idk what interface good
void getRoute(Train train, Sensor end) {
    (void) train; (void) end;
}
