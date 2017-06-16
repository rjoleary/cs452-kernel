#pragma once

#define memcpy  __builtin_memcpy
#define memset  __builtin_memset
#define memcmp  __builtin_memcmp
#define strncpy __builtin_strncpy
#define strlen  __builtin_strlen

extern "C"
void fast_memcpy(unsigned *dest, const unsigned *source, int num);

#define ASSERT(pred) do {               \
    if (__builtin_expect(!(pred), 0)) { \
        ctl::detail::assert(__FILE__, __LINE__);     \
    }                                   \
} while (false)

namespace ctl {
using size_t = decltype(sizeof(0));
namespace detail {
void assert(const char *file, int line);
}
}

class NonCopyable {
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable &operator=(const NonCopyable&) = delete;
protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;
};
