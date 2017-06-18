#include <ns.h>
#include <std.h>

void idleMain() {
    ~ctl::registerAs(ctl::Name{"Idle"});
    for (;;);
}
