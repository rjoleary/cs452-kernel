typedef decltype(sizeof(0)) size_t;

// Cannot include std.h for these functions to work.
extern "C" {
void *memset(void *s, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        *static_cast<char*>(s) = static_cast<char>(c);
    }
    return s;
}

int memcmp(const void *a, const void *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char ca = static_cast<const char*>(a)[i];
        char cb = static_cast<const char*>(b)[i];
        if (ca != cb) {
            return ca < cb ? -1 : 1;
        }
    }
    return 0;
}

char *strncpy(char *dest, const char *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
        if (!*dest) {
            break;
        }
    }
    return dest;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (*(s++)) {
        n++;
    }
    return n;
}
}

#include <bwio.h>
#include <task.h>

namespace ctl {
namespace detail {
void assert(const char *file, int line) {
    bwprintf(COM2, "Assertion failed in Tid %d, %s:%d\r\n", ctl::myTid(), file, line);
    exeunt();
}
}
}
