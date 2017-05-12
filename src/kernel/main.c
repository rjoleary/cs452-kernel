#include <bwio.h>
#include <def.h>
#include <panic.h>
#include <req.h>
#include <td.h>
#include <user/task.h>

struct Td *schedule() {
    // TODO
    return 0;
}

void activate(struct Td *td, struct Request *req) {
}

void handle(const struct Request *req) {
}

void svcHandle(unsigned id) {
    // SWI immediate value is only 24bits.
    id &= 0xffffff;
    bwprintf(COM2, "HANDLE %u\r\n", id);
    PANIC("HANDLE");
}

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    const char* buildstr(void);
    bwprintf(COM2, "%s\r\n", buildstr());

    initTds();





    volatile void **SVC_VECTOR = (void*)0x28;
    void kernel_entry(void);
    *SVC_VECTOR = &kernel_entry;

    myTid();





    PANIC("done");

    struct Request request;
    struct Td *active;
    while (1) {
        active = schedule();
        activate(active, &request);
        handle(&request);
    }

    return 0;
}
