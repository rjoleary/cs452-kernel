#ifndef USER_TYPES_H__INCLUDED
#define USER_TYPES_H__INCLUDED

namespace ctl {
template <typename T, typename Name>
class NamedType {
    T t;
public:
    using underlying_type = T;
    constexpr explicit NamedType(T t_) : t(t_){}
    constexpr NamedType() = default;
    constexpr bool operator==(const NamedType &rhs) const { return t == rhs.t; }
    constexpr bool operator<(const NamedType &rhs) const { return t < rhs.t; }
    constexpr bool operator>(const NamedType &rhs) const { return t > rhs.t; }
    T& underlying() { return t; }
    constexpr const T& underlying() const { return t; }
};

using Tid = NamedType<int, struct Tid_t>;

// There are 32 valid priorities: [0,31]
// 0: low priority
// 31: high priority
using Priority = NamedType<char, struct Priority_t>;

constexpr Priority PRIORITY_MIN{0}, PRIORITY_MAX{31};
}

#endif // USER_TYPES_H__INCLUDED
