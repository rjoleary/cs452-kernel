#include <track.h>
#include <ns.h>
#include <circularbuffer.h>
#include <itc.h>
#include <bwio.h>
#include <def.h>
#include <sensor.h>
#include <switch.h>
#include <clock.h>
#include <path_finding.h>
#include <train.h>

#include "track_node.h"
#include "track_data_new.h"

void savecur();
void restorecur();
void setpos(unsigned, unsigned);

namespace {
constexpr ctl::Name TrackServName = {"TrackM"};

// Returns the stopping distance (mm) for the given train and speed.
int stoppingDistance(int train, int speed) {
    // TODO: this is only forwards
    if (train == 63 && speed == 10) return 807;
    if (train == 63 && speed == 12) return 974;
    if (train == 71 && speed == 10) return 494;
    if (train == 71 && speed == 12) return 807;

    // All other trains
    return 1000;
}

enum class MsgType {
    Sensor,
    Switch,
    Delay,
    Route,
    SetStoppingDistance,
    ClearBrokenSwitches,
};

struct Message {
    MsgType type;
    union {
        struct {
            char module, number;
        } sensor;
        struct {
            char sw, state;
        } turnout;
        struct {
            char train, speed, sensor;
        } route;
        struct {
            int mm;
        } setStoppingDistance;
    };
};

struct Reply {
    int ticks;
};

void delayWorker() {
    auto trackMan = whoIs(TrackServName).asValue();
    auto clockServ = whoIs(names::ClockServer).asValue();
    Message msg = {MsgType::Delay};
    for (;;) {
        Reply reply;
        ~send(trackMan, msg, reply);
        ~delay(clockServ, reply.ticks);
    }
}

void sensorNotifierMain() {
    auto serv = whoIs(TrackServName).asValue();
    Message msg = {MsgType::Sensor};
    for (;;) {
        auto sens = waitTrigger();
        for (int i = 0; i < NUM_SENSOR_MODULES; ++i) {
            for (int j = 0; j < NUM_SENSORS_PER_MODULE; ++j) {
                if (sens(i, j)) {
                    msg.sensor.module = i;
                    msg.sensor.number = j;
                    ~send(serv, msg, ctl::EmptyMessage);
                }
            }
        }
    }
}

void switchNotifierMain() {
    auto serv = whoIs(TrackServName).asValue();
    Message msg = {MsgType::Switch};
    auto switches = getSwitchData();
    for (;;) {
        auto newSwitches = waitSwitchChange();
        for (int i = 0; i < NumSwitches; ++i) {
            if (switches.states[i] != newSwitches.states[i]) {
                msg.turnout.sw = switches.fromIdx(i);
                msg.turnout.state = newSwitches.states[i];
                ~send(serv, msg, ctl::EmptyMessage);
            }
        }
        switches = newSwitches;
    }
}
} // unnamed namespace

void trackMain() {
    TrackNode nodes[TRACK_MAX];
    init_tracka(nodes);

    ~registerAs(TrackServName);
    ~create(ctl::Priority(26), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);
    auto delayTid = create(ctl::Priority(26), delayWorker).asValue();

    auto switches = getSwitchData();
    auto clockServ = whoIs(ctl::names::ClockServer).asValue();

    auto prevTime = ctl::time(clockServ).asValue();
    const TrackNode *expectedNode = nullptr, *previousNode;
    auto prevDist = 0;
    auto prevVelocity = 1;

    Path path[Graph::VSize];
    int pathLength = 0;
    int pathStart = 0;

    int routingTrain = -1;
    int routingSpeed = -1;

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
            case MsgType::Delay: {
                bwputstr(COM2, "RECEIVED DELAY\r\n");
                if (routingTrain != -1) {
                    cmdSetSpeed(routingTrain, 0);
                }
                break;
            }

            case MsgType::Sensor: {
                ~reply(tid, ctl::EmptyMessage);
                const auto &currNode = 
                    nodes[msg.sensor.module*NUM_SENSORS_PER_MODULE + msg.sensor.number];
                savecur();
                bwprintf(COM2, "\033[40;1H\033[JCurrent node: %s\r\n",
                        currNode.name);

                int i = 41;
                if (expectedNode && expectedNode != &currNode) {
                    bwprintf(COM2, "\033[%d;1HUNEXPECTED NODE HIT, WANTED %s\r\n",
                            i++, expectedNode->name);
                    Path failPath[Graph::VSize];
                    NodeIdx beginIdx = previousNode->num;
                    NodeIdx endIdx = currNode.num;
                    auto failLength = dijkstra(Graph{nodes}, beginIdx, endIdx, failPath);
                    bool sensorBroken = false;
                    for (int i = 1; i < failLength-1; ++i) {
                        auto &failNode = nodes[failPath[i].nodeIdx];
                        if (failNode.type == NODE_SENSOR) {
                            failNode.type = NODE_BROKEN_SENSOR;
                            bwprintf(COM2, "\033[38;1H\033[JSensor %d broken\r\n",
                                    failNode.num);
                            sensorBroken = true;
                        }
                        else if (failNode.type == NODE_BRANCH && !sensorBroken) {
                            auto &incorrectDir = switches[failNode.num];
                            if (incorrectDir == 'C') {
                                cmdSetSwitch(failNode.num, 'S');
                                failNode.type = NODE_BROKEN_SW_ST;
                                incorrectDir = 'S';
                                bwprintf(COM2, "\033[38;1H\033[JSwitch %d broken, permenantly S\r\n",
                                        failNode.num);
                            }
                            else {
                                cmdSetSwitch(failNode.num, 'C');
                                failNode.type = NODE_BROKEN_SW_CV;
                                incorrectDir = 'C';
                                bwprintf(COM2, "\033[38;1H\033[JSwitch %d broken, permenantly C\r\n",
                                        failNode.num);
                            }
                        }
                    }
                    if (pathLength) {
                        NodeIdx beginIdx = currNode.num;
                        NodeIdx endIdx = path[pathLength - 1].nodeIdx;
                        pathLength = dijkstra(Graph{nodes}, beginIdx, endIdx, path);
                        if (pathLength == 0) {
                            bwputstr(COM2, "Cannot reroute\r\n");
                        }
                    }
                }
                if (pathLength) {
                    ASSERT(&currNode == &nodes[path[pathStart].nodeIdx]);
                    int nextSensor = pathStart+1;
                    for (; nextSensor < pathLength; ++nextSensor)
                        if (nodes[path[nextSensor].nodeIdx].type == NODE_SENSOR)
                            break;
                    for (int i = pathStart+1; i < pathLength; ++i) {
                        const auto &pathNode = path[i];
                        const auto &node = nodes[pathNode.nodeIdx];
                        if (node.type == NODE_BRANCH) {
                            if (nextSensor > i ||
                                    path[nextSensor].distance - pathNode.distance < 100) {
                                const auto &next = nodes[path[i+1].nodeIdx];
                                char wantedState = 'S';
                                if (node.edge[DIR_CURVED].dest == &next) {
                                    wantedState = 'C';
                                }
                                if (switches[node.num] != wantedState) {
                                    cmdSetSwitch(node.num, wantedState);
                                    switches[node.num] = wantedState;
                                    bwprintf(COM2, "\033[39;1H\033[JSwitch %d changed to %c\r\n",
                                            node.num, wantedState);
                                }
                            }
                        }
                    }
                    const auto sd = stoppingDistance(routingTrain, routingSpeed);
                    if (path[nextSensor].distance < sd) {
                        int ticks = (path[pathStart].distance - sd)*1000 / prevVelocity / 10;
                        bwprintf(COM2, "Setting delay for %d ticks\r\n", ticks);
                        if (tick > 0) {
                            ~reply(delayTid, Reply{ticks});
                        }
                    }
                    pathStart = nextSensor;
                    if (pathStart == pathLength) {
                        pathLength = 0;
                    }
                }
                int dist = currNode.edge[DIR_AHEAD].dist;
                auto next = currNode.edge[DIR_AHEAD].dest;
                bwprintf(COM2, "\033[%d;1HPath until next sensor:\r\n", i++);
                while (next->type != NODE_SENSOR && next->type != NODE_EXIT) {
                    bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                    auto dir = DIR_AHEAD;
                    if (next->type == NODE_BRANCH && switches[next->num] == 'C')
                        dir = DIR_CURVED;
                    dist += next->edge[dir].dist;
                    next = next->edge[dir].dest;
                }
                bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                bwprintf(COM2, "\033[%d;1HDistance: %d\r\n", i++, dist);

                auto currTime = ctl::time(clockServ).asValue();
                auto deltaTime = currTime-prevTime;
                auto velocity = (prevDist*1000)/deltaTime;
                bwprintf(COM2, "\033[%d;1HVelocity: %d\r\n", i++, velocity);
                bwprintf(COM2, "\033[%d;1HExpected time: %d\r\n", i++, (prevDist*1000)/prevVelocity);
                bwprintf(COM2, "\033[%d;1HActual time: %d\r\n", i++, deltaTime);
                expectedNode = next;
                previousNode = &currNode;
                prevTime = currTime;
                prevDist = dist;
                prevVelocity = velocity;

                restorecur();
                flush(COM2);
                break;
            }

            case MsgType::Switch: {
                ~reply(tid, ctl::EmptyMessage);
                switches[msg.turnout.sw] = msg.turnout.state;
                bwprintf(COM2, "\033[39;1H\033[JSwitch %d changed to %c\r\n",
                        msg.turnout.sw, msg.turnout.state);
                break;
            }

            case MsgType::Route: {
                // Path finding
                NodeIdx beginIdx = expectedNode->num;
                NodeIdx endIdx = msg.route.sensor;
                pathLength = dijkstra(Graph{nodes}, beginIdx, endIdx, path);
                pathStart = 0;

                if (pathLength == 0) {
                    bwputstr(COM2, "Cannot find path\r\n");
                } else {
                    for (int i = 0; i < pathLength; i++) {
                        auto nodeIdx = path[i].nodeIdx;
                        bwprintf(COM2, "Node %s, distance: %d\r\n",
                                nodes[nodeIdx].name, path[i].distance);
                        if (nodes[nodeIdx].type == NODE_BRANCH) {
                            if (nodes[nodeIdx].edge[DIR_STRAIGHT].dest
                                    == &nodes[path[i+1].nodeIdx])
                                bwprintf(COM2, "Branch straight\r\n");
                            else
                                bwprintf(COM2, "Branch curved\r\n");
                        }
                    }
                    cmdSetSpeed(msg.route.train, msg.route.speed);
                }

                // Store variables for later.
                routingTrain = msg.route.train;
                routingSpeed = msg.route.speed;

                ~reply(tid, ctl::EmptyMessage);
                break;
            }

            case MsgType::SetStoppingDistance: {
                ~reply(tid, ctl::EmptyMessage);
                break;
            }

            case MsgType::ClearBrokenSwitches: {
                ~reply(tid, ctl::EmptyMessage);
                for (int i = 0; i < 80+22; ++i) {
                    if (nodes[i].type == NODE_BROKEN_SENSOR)
                        nodes[i].type = NODE_SENSOR;
                    if (nodes[i].type == NODE_BROKEN_SW_ST
                            || nodes[i].type == NODE_BROKEN_SW_CV)
                        nodes[i].type = NODE_BRANCH;
                }
                break;
            }
        }
    }
}

void cmdRoute(int train, int speed, int sensor) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
        return;
    }
    if (speed < 0 || 14 < speed) {
        bwputstr(COM2, "Error: speed must be between 0 and 14 inclusive\r\n");
        return;
    }
    if (sensor < 0 || 79 < sensor) {
        bwputstr(COM2, "Error: sensor must be between 0 and 79 inclusive\r\n");
        return;
    }
    static auto trackMan = whoIs(TrackServName).asValue();
    Message msg{MsgType::Route};
    msg.route.train = train;
    msg.route.speed = speed;
    msg.route.sensor = sensor;
    ~send(trackMan, msg, ctl::EmptyMessage);
}

void cmdSetStoppingDistance(int mm) {
    static auto trackMan = whoIs(TrackServName).asValue();
    Message msg{MsgType::SetStoppingDistance};
    msg.setStoppingDistance.mm = mm;
    ~send(trackMan, msg, ctl::EmptyMessage);
}

void cmdClearBrokenSwitches() {
    static auto trackMan = whoIs(TrackServName).asValue();
    Message msg{MsgType::ClearBrokenSwitches};
    ~send(trackMan, msg, ctl::EmptyMessage);
}
