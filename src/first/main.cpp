#include <bwio.h>
#include <clock.h>
#include <def.h>
#include <event.h>
#include <ns.h>
#include <std.h>
#include <task.h>
#include <itc.h>
#include <io.h>
#include <train.h>

// Forward declaration.
void idleMain();
void runTerminal();
void trainManMain();
namespace io {
extern void (*uart1TxMain)();
extern void (*uart1RxMain)();
extern void (*uart2TxMain)();
extern void (*uart2RxMain)();
}

namespace ctl {
void nsMain();
void clockMain();

void firstMain() {
    ASSERT(Tid(create(Priority(1), nsMain)) == NS_TID);
    ASSERT(Tid(create(PRIORITY_MIN, idleMain)) == IDLE_TID);
    ASSERT(ctl::registerAs(Name{"First"}) == 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::uart2TxMain) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::uart2RxMain) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::uart1TxMain) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), io::uart1RxMain) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), clockMain) >= 0);
    ASSERT(create(Priority(22), trainManMain) >= 0);
    // TODO: move
    bwioServs[0] = whoIs(names::Uart1TxServer);
    bwioServs[1] = whoIs(names::Uart2TxServer);
    bwputstr(COM2, "\033[0m"); // reset special formatting

    goTrains();
    runTerminal();
}
}
