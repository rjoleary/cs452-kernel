#include <task.h>

// TODO: don't use bwio.h
#include <bwio.h>

// The test tasks are created by the first task and perform the following:
void testMain() {
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", myTid(), myParentTid());
    pass();
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", myTid(), myParentTid());
    exeunt();
}
