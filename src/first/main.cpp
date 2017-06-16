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
    ASSERT(create(Priority(1), nsMain).asValue() == NS_TID);
    ASSERT(create(PRIORITY_MIN, idleMain).asValue() == IDLE_TID);
    ~ctl::registerAs(Name{"First"});
    ~create(Priority(PRIORITY_MAX.underlying() - 2), io::uart2TxMain);
    ~create(Priority(PRIORITY_MAX.underlying() - 2), io::uart2RxMain);
    ~create(Priority(PRIORITY_MAX.underlying() - 2), io::uart1TxMain);
    ~create(Priority(PRIORITY_MAX.underlying() - 2), io::uart1RxMain);
    ~create(Priority(PRIORITY_MAX.underlying() - 2), clockMain);
    ~create(Priority(22), trainManMain);
    bwioServs[0] = ~whoIs(ctl::names::Uart1TxServer);
    bwioServs[1] = ~whoIs(ctl::names::Uart2TxServer);

    goTrains();
    runTerminal();
}
}
