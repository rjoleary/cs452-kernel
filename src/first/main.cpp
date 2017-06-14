#include <bwio.h>
#include <clock.h>
#include <def.h>
#include <event.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <itc.h>
#include <io.h>

// Forward declaration.
void idleMain();
void runTerminal();
namespace io {
extern void (*ioMainUart1)();
extern void (*ioMainUart2)();
}

namespace ctl {
void nsMain();
void clockMain();

void firstMain() {
    ASSERT(Tid(create(PRIORITY_MIN, idleMain)) == IDLE_TID);
    ASSERT(Tid(create(PRIORITY_MAX, nsMain)) == NS_TID);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::ioMainUart1) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::ioMainUart2) >= 0);
    ASSERT(create(Priority(30), clockMain) >= 0);

    runTerminal();
}
}
