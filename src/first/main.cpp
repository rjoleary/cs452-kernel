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
namespace io {
template <ctl::Names Server, ctl::Event Ev>
void txMain();
template <ctl::Names Server, ctl::Event Ev>
void rxMain();
}

namespace ctl {
void nsMain();
void clockMain();

void firstMain() {
    ASSERT(Tid(create(PRIORITY_MIN, idleMain)) == IDLE_TID);
    ASSERT(Tid(create(Priority(2), nsMain)) == NS_TID);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), 
                io::txMain<Names::Uart2Tx, Event::Uart2Tx>) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), 
                io::rxMain<Names::Uart2Rx, Event::Uart2Rx>) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), 
                io::txMain<Names::Uart1Tx, Event::Uart1Tx>) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), 
                io::rxMain<Names::Uart1Rx, Event::Uart1Rx>) >= 0);
    ASSERT(create(Priority(PRIORITY_MAX.underlying() - 2), clockMain) >= 0);
    // TODO: move
    bwioServs[0] = whoIs(Names::Uart1Tx);
    bwioServs[1] = whoIs(Names::Uart2Tx);
    bwputstr(COM2, "\033[0m"); // reset special formatting

    initTrains();
    runTerminal();
}
}
