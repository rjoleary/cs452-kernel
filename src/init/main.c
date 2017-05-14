#include <user/task.h>

// TODO: this is a no-no because user mode is calling kernel mode function.
#include <bwio.h>

void initMain() {
    bwputstr(COM2, "Init task\r\n");
    //bwprintf(COM2, "myTid=%d\r\n", myTid());
    //bwprintf(COM2, "myParentTid=%d\r\n", myParentTid());
    myTid();
    myParentTid();
}
