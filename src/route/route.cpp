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
    
    struct {
        Path path[MAX_PATH];
        Train train = INVALID_TRAIN;
    } routes[MAX_CONCURRENT_TRAINS];

    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NewRoute: {
                for (auto &route : routes) {
                    if (route.train == INVALID_TRAIN
                            || route.train == msg.train) {
                        auto length = dijkstra(Graph{trackNodes},
                                msg.start.value(),
                                msg.end.value(),
                                route.path);
                        // Return array somehow, you can't just assign it because C
                        //ctl::reply(tid, RouteReply{route.path, length});
                        (void) length;
                        goto Breakout;
                    }
                }
                // No space for new route, respond accordingly
                ctl::reply(tid, RouteReply{{}, -1});
            }
        }
Breakout: ;
    }
}

// Idk what interface good
void getRoute(Train train, Sensor start, Sensor end) {
    (void) train; (void) start; (void) end;
}
