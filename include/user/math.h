#pragma once

namespace ctl {
template <typename T>
constexpr T min(T lhs, T rhs) {
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
constexpr T max(T lhs, T rhs) {
    return lhs < rhs ? rhs : lhs;
}
}
