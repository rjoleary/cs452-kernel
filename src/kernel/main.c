#include <def.h>
#include <td.h>
#include <bwio.h>
#include <panic.h>

int main() {
    bwsetspeed(COM2, 115200);
    bwsetfifo(COM2, OFF);

    // Print the build string (date + time).
    const char* buildstr();
    bwputstr(COM2, buildstr());
    bwputstr(COM2, "\r\nHello World!\r\n");
    unsigned int num = 3;
    bwprintf(COM2, "%d", __builtin_clz(num));

    initTds();

    PANIC("THIS IS A TEST");

    return 0;
}
