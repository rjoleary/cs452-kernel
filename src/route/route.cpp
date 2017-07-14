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
    Path path[MAX_PATH];
    int length;
};

} // unnamed namespace

void routeMain() {
    TrackNode trackNodes[TRACK_MAX];
    init_tracka(trackNodes);
    
    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NewRoute: {
                RouteReply reply;
                reply.length = dijkstra(Graph{trackNodes},
                        msg.start.value(),
                        msg.end.value(),
                        reply.path);
                ctl::reply(tid, reply);
            }
        }
    }
}

// Idk what interface good
void getRoute(Train train, Sensor start, Sensor end) {
    (void) train; (void) start; (void) end;
}
