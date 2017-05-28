#ifndef USER_STD_H__INCLUDED
#define USER_STD_H__INCLUDED

namespace ctl {
using size_t = decltype(sizeof(0));

void fast_memcpy(unsigned *dest, const unsigned *src, size_t count);
#define memcpy  __builtin_memcpy
#define memset  __builtin_memset
#define memcmp  __builtin_memcmp
#define strncpy __builtin_strncpy
}

#endif // USER_STD_H__INCLUDED
