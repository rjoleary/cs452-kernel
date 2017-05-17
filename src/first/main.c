#include <task.h>

// TODO: don't use bwio.h
#include <bwio.h>

// Forward declaration.
void testMain(void);

void f() {
    bwprintf(COM2, "F Tid: %d\r\n", myTid());
}

// The first user task performs the following in order:
//  1. Creates two test tasks of lower priority
//  2. Creates two test tasks of higher priority
void firstMain() {
    create(5, f);
    bwprintf(COM2, "F called\r\n");
    /*bwprintf(COM2, "FirstUserTask: entering\r\n");
    bwprintf(COM2, "Tid: %d ParentTid: %d\r\n", myTid(), myParentTid());
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(1, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(1, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(5, testMain));
    bwprintf(COM2, "FirstUserTask: created tid %d\r\n", create(5, testMain));
    bwprintf(COM2, "FirstUserTask: exiting\r\n");*/
}
