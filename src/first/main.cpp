#include <bwio.h>
#include <task.h>

// Forward declaration.
namespace ctl {
void nsMain();
}
void rpsServerMain();
void rpsClientMain();

void firstMain() {
    bwprintf(COM2, "FirstUserTask: myTid()=%d\r\n", ctl::myTid());
    bwprintf(COM2, "FirstUserTask: created nameserver, tid %d\r\n", ctl::create(1, ctl::nsMain));
    ctl::create(5, rpsServerMain);
    ctl::create(2, rpsClientMain);
    ctl::create(2, rpsClientMain);
}
