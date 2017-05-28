#include <bwio.h>
#include <def.h>
#include <task.h>

// Forward declaration.
void rpsServerMain();
void rpsClientMain();

namespace ctl {
void nsMain();

void firstMain() {
    bwprintf(COM2, "FirstUserTask: myTid()=%d\r\n", myTid());
    bwprintf(COM2, "FirstUserTask: created nameserver, tid %d\r\n",
            create(PRIORITY_MAX, nsMain));
    create(Priority(5), rpsServerMain);
    for (unsigned i = 0; i < NUM_RPS_CLIENTS; i++) {
        create(Priority(2), rpsClientMain);
    }
}
}
