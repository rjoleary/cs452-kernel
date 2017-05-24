#include <task.h>

void idleMain() {
    while (1) {
        ctl::pass();
    }
}
