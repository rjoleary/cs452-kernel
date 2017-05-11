#ifndef SWAP_H__INCLUDED
#define SWAP_H__INCLUDED

#define MAKE_SWAP(T, Name)					\
inline void Swap##Name(T *lhs, T *rhs) {	\
	T temp = *lhs;							\
	*lhs = *rhs;							\
	*rhs = temp;							\
}

MAKE_SWAP(int, Int)

#endif // SWAP_H__INCLUDED
