#include <def.h>
#include <td.h>
#include <bwio.h>

const char* buildstr();

int main() {
    // Print the build string (date + time).
    bwputstr(COM2, buildstr());
    bwputstr(COM2, "\r\nHello World!\r\n");

    // Initialize task descriptors.
    struct Td tds[NUM_TD];
    for (int i = 0; i < NUM_TD; i++) {
        tds[i].tid = -1;
    }

    return 0;
}
