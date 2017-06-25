#include <ns.h>
#include <circularbuffer.h>
#include <itc.h>
#include <bwio.h>
#include <clock.h>
#include <def.h>

#include "track_data_new.h"

void savecur();
void restorecur();
void setpos(unsigned, unsigned);

void trackMain() {
    track_node nodes[TRACK_MAX];
    init_tracka(nodes);
}
