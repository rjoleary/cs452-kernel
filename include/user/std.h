#ifndef USER_STD_H__INCLUDED
#define USER_STD_H__INCLUDED

namespace ctl {
using size_t = decltype(sizeof(0));

#define memcpy  __builtin_memcpy
#define memset  __builtin_memset
#define memcmp  __builtin_memcmp
#define strncpy __builtin_strncpy
}

extern "C"
void fast_memcpy(unsigned *dest, const unsigned *source, int num);

#endif // USER_STD_H__INCLUDED
