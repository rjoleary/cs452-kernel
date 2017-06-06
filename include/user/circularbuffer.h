#pragma once

#include <std.h>

namespace ctl {
template <typename Data, ctl::size_t MaxSize>
class CircularBuffer {
    static_assert(std::is_trivial<Data>::value, "Data must be trivial");
    Data entries[MaxSize];
    Data *read = &entries[0], *write = &entries[0];
    size_t size = 0;
public:
    const Data& pop() {
        ASSERT(--size >= 0);
        const auto &ret = *read;
        ++read;
        if (read == &entries[MaxSize])
            read = &entries[0];
        return ret;
    }
    void push(const Data &data) {
        ASSERT(++size <= MaxSize);
        *write = data;
        ++write;
        if (write == &entries[MaxSize])
            write = &entries[0];
    }
    bool empty() const {
        return size == 0;
    }
};
}
