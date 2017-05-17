#include <task.h>
#include "../../include/panic.h"

// TODO: don't use bwio.h
#include <bwio.h>

// The test tasks are created by the first task and perform the following:
void testMain() {
    int tid = myTid();
    int pTid = myParentTid();
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", tid, pTid);
    pass();
    bwprintf(COM2, "TestUserTask: myTid()=%d, myParentTid()=%d\r\n", myTid(), myParentTid());
}
