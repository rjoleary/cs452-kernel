#include <bwio.h>
#include <def.h>
#include <panic.h>
#include <req.h>
#include <td.h>

struct Td *schedule() {
    // TODO
    return 0;
}

void activate(struct Td *td, struct Request *req) {
}

void handle(const struct Request *req) {
    // TODO
}

int main() {
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);

    // Print the build string (date + time).
    const char* buildstr();
    bwputstr(COM2, buildstr());
    bwputstr(COM2, "\r\nHello World!\r\n");
    unsigned int num = 3;
    bwprintf(COM2, "%d", __builtin_clz(num));

    initTds();

    struct Request request;
    struct Td *active;
    while (1) {
        active = schedule();
        activate(active, &request);
        handle(&request);
    }

    return 0;
}
