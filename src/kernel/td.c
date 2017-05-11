#include <def.h>
#include <td.h>

static struct Td tds[NUM_TD];

void initTds() {
    for (int i = 0; i < NUM_TD; i++) {
        tds[i].tid = -1;
    }
}

struct Td* getTdByTid(Tid tid) {
    for (int i = 0; i < NUM_TD; i++) {
        if (tds[i].tid == tid) {
            return &tds[i];
        }
    }
    return 0;
}
