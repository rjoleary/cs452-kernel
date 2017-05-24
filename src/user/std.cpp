typedef decltype(sizeof(0)) size_t;

extern "C" {
void *memcpy(void *dest, const void *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        static_cast<char*>(dest)[i] = static_cast<const char*>(src)[i];
    }
    return dest;
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
