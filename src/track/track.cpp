#include <track.h>
#include <ns.h>
#include <circularbuffer.h>
#include <itc.h>
#include <bwio.h>
#include <def.h>
#include <sensor.h>

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
        } trunout;
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
}
} // unnamed namespace

void trackMain() {
    TrackNode nodes[TRACK_MAX];
    init_tracka(nodes);

    ~registerAs(TrackServName);
    ~create(ctl::Priority(26), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);

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

                bwprintf(COM2, "\033[41;1HPath until next sensor:\r\n");
                auto next = currNode.edge[DIR_AHEAD].dest;
                int i = 42;
                int dist = currNode.edge[DIR_AHEAD].dist;
                while (next->type != NODE_SENSOR && next->type != NODE_EXIT) {
                    bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                    dist += next->edge[DIR_AHEAD].dist;
                    next = next->edge[DIR_AHEAD].dest;
                }
                bwprintf(COM2, "\033[%d;1H%s\r\n", i++, next->name);
                bwprintf(COM2, "\033[%d;1HDistance: %d\r\n", i++, dist);

                restorecur();
                flush(COM2);
                break;
            }
            case MsgType::Switch:
                break;
        }
    }
}
