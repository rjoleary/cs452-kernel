#pragma once

#define memcpy  __builtin_memcpy
#define memset  __builtin_memset
#define memcmp  __builtin_memcmp
#define strncpy __builtin_strncpy
#define strlen  __builtin_strlen

extern "C"
void fast_memcpy(unsigned *dest, const unsigned *source, int num);

#define STR2(x) #x
#define STR(x) STR2(x)
#define ASSERT(pred) do {                        \
    if (__builtin_expect(!(pred), 0)) {          \
        ctl::assert(__FILE__ ":" STR(__LINE__)); \
    }                                            \
} while (false)

namespace ctl {
using size_t = decltype(sizeof(0));
void assert(const char *description);

template <typename T>
void swap(T &lhs, T &rhs) {
    T temp = lhs;
    lhs = rhs;
    rhs = temp;
}

template <typename T>
constexpr T min(T lhs, T rhs) {
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
constexpr T max(T lhs, T rhs) {
    return lhs < rhs ? rhs : lhs;
}

class NonCopyable {
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable &operator=(const NonCopyable&) = delete;
protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;
};
}

