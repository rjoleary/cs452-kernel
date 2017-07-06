#include <def.h>
#include <task.h>
#include <ns.h>
#include <bwio.h>
#include <train.h>
#include <io.h>
#include <std.h>
#include <parse.h>
#include <event.h>
#include <clock.h>
#include <sensor.h>
#include <switch.h>
#include <track.h>

using namespace ctl;

// The number of bytes allocated for the command buffer.
#define MAX_CMDSZ 70

// Character to quit and return to RedBoot: CTRL+D
#define QUIT_CHAR 0x04

// Character for stop: TAB
#define STOP_CHAR '\t'

static const char prompt[] = "% ";

const char *LAYOUT =
        R"(00:00.0          Idle Time: 0%    )""\r\n"
        R"(                                  )""\r\n"
        R"(|-----#---#------------------     )""\r\n"
        R"(     /12 /11                 \    )""\r\n"
        R"(|---#   #-------#-----#-------#   )""\r\n"
        R"(   /4  /14     13\ - /10      9\  )""\r\n"
        R"(|--   /           \|/           \ )""\r\n"
        R"(     |          156#155          |)""\r\n"
        R"(     |      o      |      o      |)""\r\n"
        R"(     |          153#154          |)""\r\n"
        R"(|--   \           /|\           / )""\r\n"
        R"(   \   \         / - \         /  )""\r\n"
        R"(|---#   #-------#-----#-------#   )""\r\n"
        R"(    1\ 15\      16    17     /8   )""\r\n"
        R"(|-----#   ---#-----------#---     )""\r\n"
        R"(      2\     6\         /7        )""\r\n"
        R"(|-------#------#-------#--------| )""\r\n"
        R"(        3      18      5          )""\r\n"
        R"(                                  )""\r\n"
        R"(Run help for a list of commands.  )""\r\n";

void printLayout() {
    bwputstr(COM2, LAYOUT);
}

// Set position of cursor to (row, col).
void setpos(unsigned row, unsigned col) {
    char str[] = "\033[%d;%dH";
    bwprintf(COM2, str, row, col);
}

// Save cursor position.
void savecur() {
    char save[] = "\033[s\033[?25l";
    bwputstr(COM2, save);
}

// Restore cursor position.
void restorecur() {
    char restore[] = "\033[u\033[?25h";
    bwputstr(COM2, restore);
}

void timerMain() {
    ~registerAs(Name{"Timer"});

    int tenths = 0;
    auto clock = whoIs(names::ClockServer).asValue();
    for (;;) {
        tenths++;
        ~delayUntil(clock, tenths * 10);
        savecur();
        setpos(1, 1);
        bwprintf(COM2, "%02d:%02d.%d",
            tenths / 10 / 60, // minutes
            tenths / 10 % 60, // seconds
            tenths % 10); // tenths of a second
        restorecur();
        flush(COM2);
    }
}

void idleCounterMain() {
    ~registerAs(Name{"Counter"});

    int seconds = 0;
    auto clock = whoIs(names::ClockServer).asValue();
    for (;;) {
        seconds++;
        ~delayUntil(clock, seconds * 100);
        savecur();
        setpos(1, 29);
        ctl::TaskInfo ti;
        ASSERT(ctl::taskInfo(IDLE_TID, &ti) == ctl::Error::Ok);
        bwprintf(COM2, "    \b\b\b\b%d%%", ti.userPercent);
        restorecur();
        flush(COM2);
    }
}

void runTerminal() {
    // TODO: Doesn't work
    ~registerAs(Name{"Term"});

    // Reset special formatting.
    bwputstr(COM2, "\033[0m");

    // Clear display.
    setpos(1, 1);
    char clear[] = "\033[J";
    bwputstr(COM2, clear);

    TrainServer trainServer;
    trainServer.goTrains();

    // Print initial text.
    printLayout();
    setpos(1, 11);
    bwputstr(COM2, "\033[32m GO \033[37m");

    // Create timer and idle task counter.
    ~create(Priority(20), timerMain);
    ~create(Priority(20), idleCounterMain);

    // Create switch task
    ~create(Priority(25), switchMan);
    setupSwitches();

    auto clock = whoIs(names::ClockServer).asValue();
    // Delay for switches to finish initializing
    delay(clock, 600);

    // Create sensors task.
    ~create(Priority(25), sensorsMain);

    // Create track manager task.
    ~create(Priority(25), trackMain);

    bool isStopped = false;
    unsigned cmdsz = 0;
    char cmdbuf[MAX_CMDSZ+1];
    setpos(35, 1);
    bwputstr(COM2, prompt);

    Tid rx = whoIs(names::Uart2RxServer).asValue();
    for (;;) {
        flush(COM2);
        // TODO: don't assert on corrupt data
        int c = io::getc(rx).asValue();

        switch (c) {
        case QUIT_CHAR: // quit
            goto Return;
            
        case STOP_CHAR: // emergency stop
            savecur();
            setpos(1, 11);
            if (isStopped) {
                bwputstr(COM2, "\033[32m GO \033[37m");
                trainServer.goTrains();
            } else {
                bwputstr(COM2, "\033[31mSTOP\033[37m");
                trainServer.stopTrains();
            }
            restorecur();
            isStopped = !isStopped;
            break;
        case '\b': // backspace
            if (cmdsz) { // prevent underflow
                cmdsz--;
                bwputstr(COM2, "\b \b");
            }
            break;
        case '\r': // enter
            setpos(21,1);
            bwputstr(COM2, "\033[J");
            flush(COM2);
            cmdbuf[cmdsz] = '\0'; // null-terminate
            if (parseCmd(cmdbuf)) {
                flush(COM2);
                goto Return;
            }
            cmdsz = 0;
            setpos(35,1);
            bwputstr(COM2, prompt);
            break;
        default:
            if (cmdsz != MAX_CMDSZ) { // prevent overflow
                cmdbuf[cmdsz++] = c;
                bwputc(COM2, c);
            }
        }
    }
Return:
    // TODO: Only quit when all flying transactions are done
    // May burn out solenoid if quit too quickly
    bwputstr(COM2, "Waiting for switches...\r\n");
    delay(clock, 150);
}
