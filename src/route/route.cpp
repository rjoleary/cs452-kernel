#include <itc.h>
#include <def.h>
#include <model.h>
#include <sensor.h>
#include <train.h>

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
    Path[MAX_PATH] path;
    int length;
};

struct Graph {
    TrackNode *vertices;

    static constexpr ctl::size_t VSize = TRACK_MAX;

    auto adjacentN(int idx) const {
        switch (vertices[idx].type) {
            case NODE_BRANCH: return 2;
            case NODE_SENSOR:
            case NODE_MERGE:
            case NODE_BROKEN_SENSOR:
            case NODE_BROKEN_SW_ST:
            case NODE_BROKEN_SW_CV:
            case NODE_ENTER: return 1;
            case NODE_EXIT: return 0;
            default: return 0; // TODO: make enum class
        }
    }
    const TrackEdge *adjacent(int idx, int i) const {
        if (vertices[idx].type == NODE_BROKEN_SW_CV)
            return &vertices[idx].edge[i+1];
        return &vertices[idx].edge[i];
    }
    auto dest(const TrackEdge *edge) const {
        return edge->dest - vertices;
    }
    auto weight(const TrackEdge *edge) const {
        return edge->dist;
    }
};

} // unnamed namespace

void routeMain() {
    constexpr auto InvalidTrain = -1;

    TrackNode trackNodes[TRACK_MAX];
    init_tracka(trackNode);
    
    struct {
        Path path[MAX_PATH];
        ctl::Tid train = INVALID_TRAIN;
    } routes[MAX_CONCURRENT_TRAINS];

    for (;;) {
        ctl::Tid tid;
        Message msg;

        ctl::receive(&tid, msg);

        switch (msg.type) {
            case MsgType::NewRoute: {
                for (auto &route : routes) {
                    if (route.trainTid == INVALID_TID
                            || route.train == msg.train) {
                        auto length = dijkstra(Graph(trackNodes),
                                route.path,
                                msg.start.value(),
                                msg.end.value(),
                                route.path);
                        ctl::reply(tid, RouteReply{route.path, length});
                        goto Breakout;
                    }
                }
                // No space for new route, respond accordingly
                ctl::reply(tid, RouteReply{{}, -1});
            }
        }
Breakout:
    }
}

// Idk what interface good
void getRoute(Train train, Sensor start, Sensor end) {

}
