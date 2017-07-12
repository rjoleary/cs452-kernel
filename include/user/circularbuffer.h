#pragma once

#include <std.h>
#include <type_traits>

namespace ctl {
// Overflow policy is to drop characters.
template <typename Data, Size MaxSize>
class CircularBuffer {
    static_assert(std::is_trivial<Data>::value, "Data must be trivial");
    Data entries[MaxSize];
    Size begin = 0, end = 0, size_ = 0;
public:
    const Data& pop() {
        ASSERT(!empty());
        Data &data = entries[begin];
        begin = (begin + 1) % MaxSize;
        size_--;
        return data;
    }
    void push(const Data &data) {
        ASSERT(!full());
        entries[end] = data;
        end = (end + 1) % MaxSize;
        size_++;
    }
    bool empty() const {
        return size_ == 0;
    }
    bool full() const {
        return size_ == MaxSize;
    }
    Size size() const {
        return size_;
    }
    const Data& front() {
        ASSERT(!empty());
        return entries[begin];
    }
};
}
