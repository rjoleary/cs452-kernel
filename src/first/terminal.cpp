#include <task.h>
#include <ns.h>
#include <bwio.h>
#include <train.h>
#include <io.h>
#include <std.h>
#include <parse.h>

using namespace ctl;

// The number of bytes allocated for the command buffer.
#define MAX_CMDSZ 70

// Character to quit and return to RedBoot: CTRL+D
#define QUIT_CHAR 0x04

// Character for stop: ESC
#define STOP_CHAR 0x1b

static const char prompt[] = "% ";

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

void runTerminal() {
    bool isStopped;
    unsigned cmdsz = 0;
    char cmdbuf[MAX_CMDSZ+1];

    Tid io = whoIs(Names::IoServerUart2);
    ASSERT(io.underlying() >= 0);
    for (;;) {
        int c = io::getc(io, COM1);
        ASSERT(c >= 0);

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
