#include <bwio.h>
#include <ns.h>
#include <task.h>

void rpsServerMain() {
    ctl::registerAs("rps");
    bwprintf(COM2, "Registered rps server as tid %d\r\n", ctl::myTid());
}

void rpsClientMain() {
    Tid server = ctl::whoIs("rps");
    bwprintf(COM2, "Found rps server at tid %d\r\n", server);
}
