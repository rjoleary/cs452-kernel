#include <track.h>
#include <ns.h>
#include <circularbuffer.h>
#include <itc.h>
#include <bwio.h>
#include <def.h>
#include <sensor.h>
#include <switch.h>
#include <clock.h>

#include "track_node.h"
#include "track_data_new.h"

void savecur();
void restorecur();
void setpos(unsigned, unsigned);

namespace {
constexpr ctl::Name TrackServName = {"TrackM"};

enum class MsgType {
    Sensor, Switch
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
    };
};

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

    auto switches = getSwitchData();
    auto clockServ = whoIs(ctl::names::ClockServer).asValue();
    auto prevTime = ctl::time(clockServ).asValue();
    TrackNode *expectedNode = nullptr;
    auto prevDist = 0;

    for (;;) {
        ctl::Tid tid;
        Message msg;
        ~receive(&tid, msg);
        switch (msg.type) {
            case MsgType::Sensor: {
                ~reply(tid, ctl::EmptyMessage);
                const auto &currNode = 
                    nodes[msg.sensor.module*NUM_SENSORS_PER_MODULE + msg.sensor.number];
                savecur();
                bwprintf(COM2, "\033[40;1H\033[JCurrent node: %s\r\n",
                        currNode.name);

                auto next = currNode.edge[DIR_AHEAD].dest;
                int i = 41;
                if (expectedNode && expectedNode != &currNode)
                    bwprintf(COM2, "\033[%d;1HUNEXPECTED NODE HIT, WANTED %s\r\n",
                            i++, expectedNode->name);
                int dist = currNode.edge[DIR_AHEAD].dist;
                bwprintf(COM2, "\033[%d;1HPath until next sensor:\r\n", i++);
                while (next->type != NODE_SENSOR && next->type != NODE_EXIT) {
                    bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                    auto dir = DIR_AHEAD;
                    if (next->type == NODE_BRANCH) {
                        if (switches[next->num] == 'C')
                            dir = DIR_CURVED;
                    }
                    dist += next->edge[dir].dist;
                    next = next->edge[dir].dest;
                }
                expectedNode = next;
                bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                bwprintf(COM2, "\033[%d;1HDistance: %d\r\n", i++, dist);

                auto currTime = ctl::time(clockServ).asValue();
                auto velocity = (prevDist*1000)/(currTime - prevTime);
                prevTime = currTime;
                prevDist = dist;
                bwprintf(COM2, "\033[%d;1HVelocity: %d\r\n", i++, velocity);

                restorecur();
                flush(COM2);
                break;
            }
            case MsgType::Switch:
                ~reply(tid, ctl::EmptyMessage);
                switches[msg.turnout.sw] = msg.turnout.state;
                bwprintf(COM2, "\033[39;1H\033[JSwitch %d changed to %c\r\n",
                        msg.turnout.sw, msg.turnout.state);
                break;
        }
    }
}
