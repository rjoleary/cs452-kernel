#include "std.h"

namespace ctl {
void fast_memcpy(unsigned *dest, const unsigned *src, size_t n) {
    /*if (count == 0) return;
    auto n = (count+7)/8;
    switch (count % 8) {
        case 0: do { *dest++ = *src++;
        case 7:      *dest++ = *src++;
        case 6:      *dest++ = *src++;
        case 5:      *dest++ = *src++;
        case 4:      *dest++ = *src++;
        case 3:      *dest++ = *src++;
        case 2:      *dest++ = *src++;
        case 1:      *dest++ = *src++;
                } while (--n > 0);
    }*/
    for (size_t i = 0; i < n/4; i++) {
        static_cast<unsigned*>(dest)[i] = static_cast<const unsigned*>(src)[i];
    }
}
}

using size_t = ctl::size_t;

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
}
