#include <def.h>
#include <td.h>

int main() {
    // Initialize task descriptors.
    struct Td tds[NUM_TD];
    for (int i = 0; i < NUM_TD; i++) {
        tds[i].tid = -1;
    }
    
    return 0;
}
