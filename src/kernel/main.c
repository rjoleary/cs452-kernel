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

void svcHandle() {
    bwputstr(COM2, "HANDLE\r\n");
    PANIC("HANDLE");
}

int main() {
    // Print the build string (date + time).
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);
    const char* buildstr();
    bwprintf(COM2, "%s\r\n", buildstr());

    initTds();





    volatile void **SVC_VECTOR = (void*)0x28;
    *SVC_VECTOR = &svcHandle;

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
