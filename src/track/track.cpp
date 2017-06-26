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

void sensorNotifierMain() {

}

void switchNotifierMain() {
}
} // unnamed namespace

void trackMain() {
    track_node nodes[TRACK_MAX];
    init_tracka(nodes);

    ~registerAs(TrackServName);
    ~create(ctl::Priority(26), sensorNotifierMain);
    ~create(ctl::Priority(26), switchNotifierMain);

    for (;;) {
        auto sens = waitTrigger();
        for (int i = 0; i < NUM_SENSOR_MODULES; ++i) {
            for (int j = 0; j < NUM_SENSORS_PER_MODULE; ++j) {
                if (sens(i, j)) {
                    const auto &currNode = nodes[i*NUM_SENSORS_PER_MODULE + j];
                    savecur();
                    bwprintf(COM2, "\033[40;1H\033[JCurrent node: %s\r\n",
                            currNode.name);
                    
                    bwprintf(COM2, "\033[41;1HPath until next sensor:\r\n");
                    auto curr = currNode.edge[DIR_AHEAD].dest;
                    int i = 42;
                    int dist = curr->edge[DIR_AHEAD].dist;
                    while (curr->type != NODE_SENSOR && curr->type != NODE_EXIT) {
                        bwprintf(COM2, "\033[%d;1H%s\r\n", i++, curr->name);
                        curr = curr->edge[DIR_AHEAD].dest;
                        dist += curr->edge[DIR_AHEAD].dist;
                    }
                    bwprintf(COM2, "\033[%d;1H%s\r\n", i++, curr->name);
                    bwprintf(COM2, "\033[%d;1HDistance: %d\r\n", i++, dist);

                    restorecur();
                    flush(COM2);
                }
            }
        }
    }
}
