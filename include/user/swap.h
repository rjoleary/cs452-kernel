// Implementation of swap.
#pragma once

namespace ctl {
template <typename T>
void swap(T &lhs, T &rhs) {
    T temp = lhs;
    lhs = rhs;
    rhs = temp;
}
}
