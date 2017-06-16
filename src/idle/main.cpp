#include <ns.h>
#include <std.h>

void idleMain() {
    ASSERT(ctl::registerAs(ctl::Name{"Idle"}) == 0);
    for (;;);
}
