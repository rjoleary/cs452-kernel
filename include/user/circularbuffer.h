#pragma once

#include <std.h>
#include <type_traits>

namespace ctl {
// Overflow policy is to drop characters.
template <typename Data, size_t MaxSize>
class CircularBuffer {
    static_assert(std::is_trivial<Data>::value, "Data must be trivial");
    Data entries[MaxSize];
    size_t begin = 0, end = 0, size = 0;
public:
    const Data& pop() {
        ASSERT(!empty());
        Data &data = entries[begin];
        begin = (begin + 1) % MaxSize;
        size--;
        return data;
    }
    void push(const Data &data) {
        ASSERT(!full());
        entries[end] = data;
        end = (end + 1) % MaxSize;
        size++;
    }
    bool empty() const {
        return size == 0;
    }
    bool full() const {
        return size == MaxSize;
    }
};
}
