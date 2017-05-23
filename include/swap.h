// Implementation of swap.

#ifndef SWAP_H__INCLUDED
#define SWAP_H__INCLUDED

namespace ctl {
template <typename T>
void swap(T &lhs, T &rhs) {
    T temp = lhs;
    lhs = rhs;
    rhs = temp;
}
}

#endif // SWAP_H__INCLUDED
