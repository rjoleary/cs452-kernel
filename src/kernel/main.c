#include <def.h>
#include <td.h>
#include <bwio.h>

int main() {
    // Print the build string (date + time).
    const char* buildstr();
    bwputstr(COM2, buildstr());
    bwputstr(COM2, "\r\nHello World!\r\n");
    unsigned int num = 3;
    bwprintf(COM2, "%d", __builtin_clz(num));

    initTds();

    return 0;
}
