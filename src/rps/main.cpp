#include <bwio.h>
#include <ns.h>
#include <task.h>

void rpsServerMain() {
    ctl::registerAs(ctl::Names::RpsServer);
    bwprintf(COM2, "Registered rps server as tid %d\r\n", ctl::myTid());
}

void rpsClientMain() {
    auto server = ctl::Tid(ctl::whoIs(ctl::Names::RpsServer));
    bwprintf(COM2, "Found rps server at tid %d\r\n", server);
}
