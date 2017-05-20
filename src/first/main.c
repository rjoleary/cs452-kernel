#include <bwio.h>
#include <task.h>

// Forward declaration.
void testMain(void);

// The first user task performs the following in order:
//  1. Creates two test tasks of lower priority
//  2. Creates two test tasks of higher priority
void firstMain() {
    bwprintf(COM2, "FirstUserTask: entering\r\n");
    bwprintf(COM2, "FirstUserTask: myTid()=%d\r\n", myTid());
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(1, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(1, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(5, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(5, testMain));
    bwprintf(COM2, "FirstUserTask: exiting\r\n");
}
