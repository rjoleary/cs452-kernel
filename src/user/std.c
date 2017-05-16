#include <std.h>

void *memcpy(void *dest, const void *src, unsigned n) {
    return __builtin_memcpy(dest, src, n);
}

void *memmove(void *dest, const void *src, unsigned n) {
    return __builtin_memmove(dest, src, n);
}
