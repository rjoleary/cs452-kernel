#include <int.h>

// Cannot include std.h for these functions to work.
extern "C" {
void *memcpy(void *dest, const void *src, Size n) {
    for (Size i = 0; i < n; i++) {
        *(static_cast<char*>(dest) + i) = *(static_cast<const char*>(src) + i);
    }
    return dest;
}

void *memset(void *s, int c, Size n) {
    for (Size i = 0; i < n; i++) {
        *static_cast<char*>(s) = static_cast<char>(c);
    }
    return s;
}

int memcmp(const void *a, const void *b, Size n) {
    for (Size i = 0; i < n; i++) {
        char ca = static_cast<const char*>(a)[i];
        char cb = static_cast<const char*>(b)[i];
        if (ca != cb) {
            return ca < cb ? -1 : 1;
        }
    }
    return 0;
}

char *strncpy(char *dest, const char *src, Size n) {
    for (Size i = 0; i < n; i++) {
        dest[i] = src[i];
        if (!*dest) {
            break;
        }
    }
    return dest;
}

Size strlen(const char *s) {
    Size n = 0;
    while (*(s++)) {
        n++;
    }
    return n;
}
}

#include <bwio.h>
#include <task.h>
#include <ts7200.h>

namespace ctl {
void assert(const char *desc) {
    useBusyWait = true;
    auto uartState = *(volatile unsigned*)(UART2_BASE + UART_CTLR_OFFSET);
    *(volatile unsigned*)(UART2_BASE + UART_CTLR_OFFSET) = UARTEN_MASK;
    bwprintf(COM2, "Assertion failed in Tid %d, %s\r\n", ctl::myTid(), desc);
    useBusyWait = false;
    *(volatile unsigned*)(UART2_BASE + UART_CTLR_OFFSET) = uartState;
    exeunt();
}
}
