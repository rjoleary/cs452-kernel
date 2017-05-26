#ifndef USER_DEF_H__INCLUDED
#define USER_DEF_H__INCLUDED

#include "types.h"

namespace ctl {
constexpr Tid FIRST_TID{0},
          NS_TID{1},
          INVALID_TID{-1};
constexpr Priority FIRST_PRI{15};
};

#endif // USER_DEF_H__INCLUDED
