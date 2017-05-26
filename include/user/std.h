#ifndef USER_STD_H__INCLUDED
#define USER_STD_H__INCLUDED

typedef decltype(sizeof(0)) size_t;

#define memcpy  __builtin_memcpy
#define memcmp  __builtin_memcmp
#define strncpy __builtin_strncpy

#endif // USER_STD_H__INCLUDED
