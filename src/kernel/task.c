#include <task.h>
#include <bwio.h>

Tid kCreate(Priority priority, void (*code)()) {
    // TODO: implement
    bwprintf(COM2, "SYSCALL: (priority=%d, code=0x%x)\r\n", priority, code);
    return -1;
}

Tid kMyTid(void) {
    // TODO: implement
    bwputstr(COM2, "SYSCALL: myTid\r\n");
    return -1;
}

Tid kMyParentTid(void) {
    // TODO: implement
    bwputstr(COM2, "SYSCALL: myParentTid\r\n");
    return -1;
}

void kPass(void) {
    // TODO: implement
    bwputstr(COM2, "SYSCALL: pass\r\n");
}

void kExeunt(void) {
    // TODO: implement
    bwputstr(COM2, "SYSCALL: exeunt\r\n");
}

void kDestroy(void) {
    // TODO: implement
    bwputstr(COM2, "SYSCALL: destroy\r\n");
}
