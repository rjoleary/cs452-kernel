using size_t = decltype(sizeof(0));

extern "C" {
void *memcpy(void *dest, const void *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        static_cast<char*>(dest)[i] = static_cast<const char*>(src)[i];
    }
    return dest;
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
