#include <bwio.h>
#include <task.h>

// The test tasks are created by the first task and perform the following:
void testMain() {
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", ctl::myTid(), ctl::myParentTid());
    ctl::pass();
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", ctl::myTid(), ctl::myParentTid());
    ctl::exeunt();
}
