#include <def.h>
#include <td.h>
#include <bwio.h>

int main() {
    // Initialize task descriptors.
    bwputstr(COM2, "This is a test");
    struct Td tds[NUM_TD];
    for (int i = 0; i < NUM_TD; i++) {
        tds[i].tid = -1;
    }
    
    return 0;
}
