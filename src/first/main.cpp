#include <bwio.h>
#include <task.h>

// Forward declaration.
namespace ctl {
void nsMain();
}

void firstMain() {
    bwprintf(COM2, "FirstUserTask: entering\r\n");
    bwprintf(COM2, "FirstUserTask: myTid()=%d\r\n", ctl::myTid());
    bwprintf(COM2, "FirstUserTask: created nameserver, tid %d\r\n", ctl::create(1, ctl::nsMain));
    bwprintf(COM2, "FirstUserTask: exiting\r\n");
}
