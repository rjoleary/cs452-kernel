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
#include <callibration.h>

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
    Track();
    ASSERT(create(Priority(1), nsMain).asValue() == NS_TID);
    ASSERT(create(PRIORITY_MIN, idleMain).asValue() == IDLE_TID);
    ~ctl::registerAs(Name{"First"});
    ~create(Priority(27), io::uart2TxMain);
    bwioServs[1] = ~whoIs(ctl::names::Uart2TxServer);
    ~create(Priority(27), io::uart2RxMain);
    ~create(Priority(29), io::uart1TxMain);
    bwioServs[0] = ~whoIs(ctl::names::Uart1TxServer);
    ~create(Priority(29), io::uart1RxMain);
    ~create(Priority(29), clockMain);
    ~create(Priority(25), trainManMain);
    CallibrationServer::create();

    runTerminal();
}
}
