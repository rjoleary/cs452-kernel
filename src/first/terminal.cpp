#include <task.h>
#include <ns.h>
#include <bwio.h>
#include <train.h>
#include <io.h>
#include <std.h>
#include <parse.h>
#include <event.h>
#include <clock.h>

using namespace ctl;

// The number of bytes allocated for the command buffer.
#define MAX_CMDSZ 70

// Character to quit and return to RedBoot: CTRL+D
#define QUIT_CHAR 0x04

// Character for stop: ESC
#define STOP_CHAR 0x1b

static const char prompt[] = "% ";

void printLayout() {
    bwputstr(COM2,
        "00:00.0   STOP   Built ??:??:?? ??? ?? ????\r\n"
        "                                           \r\n"
        "Longest loop: 0 us                         \r\n"
        "Current loop: 0 us                         \r\n"
        "                                           \r\n"
        "Sensor triggers          Switches          \r\n"
        "  ________                  1-U   12-U     \r\n"
        "  ________                  2-U   13-U     \r\n"
        "  ________                  3-U   14-U     \r\n"
        "  ________                  4-U   15-U     \r\n"
        "  ________                  5-U   16-U     \r\n"
        "  ________                  6-U   17-U     \r\n"
        "  ________                  7-U   18-U     \r\n"
        "  ________                  8-U  153-U     \r\n"
        "  ________                  9-U  154-U     \r\n"
        "  ________                 10-U  155-U     \r\n"
        "  ________                 11-U  156-U     \r\n"
        "                                           \r\n"
        "Run help for a list of commands.           \r\n"
    );
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
    int tenths = 0;
    auto clock = whoIs(names::ClockServer).asValue();
    for (;;) {
        tenths++;
        ~delayUntil(clock, tenths);
        // TODO: two printers currently interferes
        /*savecur();
        setpos(1, 1);
        bwprintf(COM2, "%02d:%02d.%d", 
            tenths / 10 / 60, // minutes
            tenths / 10 % 60, // seconds
            tenths % 10); // tenths of a second
        restorecur();*/
    }
}

void runTerminal() {
    // Reset special formatting.
    bwputstr(COM2, "\033[0m");

    // Clear display.
    setpos(1, 1);
    char clear[] = "\033[J";
    bwputstr(COM2, clear);

    // Print initial text.
    printLayout();
    savecur();
    setpos(1, 11);
    bwputstr(COM2, "\033[32m GO \033[37m");
    restorecur();
    bwputstr(COM2, prompt);

    // Create timer.
    // Timer must be higher priority than terminal, otherwise output gets jumbled.
    ~create(Priority(28), timerMain);

    bool isStopped = true;
    unsigned cmdsz = 0;
    char cmdbuf[MAX_CMDSZ+1];

    Tid io = whoIs(names::Uart2RxServer).asValue();
    ASSERT(io.underlying() >= 0);
    for (;;) {
        // TODO: don't assert on corrupt data
        int c = io::getc(io).asValue();

        switch (c) {
        case QUIT_CHAR: // quit
            return;
        case STOP_CHAR: // emergency stop
            savecur();
            setpos(1, 11);
            if (isStopped) {
                bwputstr(COM2, "\033[32m GO \033[37m");
                goTrains();
            } else {
                bwputstr(COM2, "\033[31mSTOP\033[37m");
                stopTrains();
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
            bwputc(COM2, '\r');
            bwputc(COM2, '\n');
            cmdbuf[cmdsz] = '\0'; // null-terminate
            if (parseCmd(cmdbuf)) {
                return;
            }
            cmdsz = 0;
            bwputstr(COM2, prompt);
            break;
        default:
            if (cmdsz != MAX_CMDSZ) { // prevent overflow
                cmdbuf[cmdsz++] = c;
                bwputc(COM2, c);
            }
        }
    }
}
