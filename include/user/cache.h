#pragma once

#include <std.h>

// Key-value cache
template <ctl::size_t Cap, typename K, typename V>
class Cache {
    K keys[Cap];
    V values[Cap];
    ctl::size_t size;
public:
    typedef ctl::size_t CacheIndex;

    // Returns the cache index or -1 if full
    CacheIndex getIdx(const K &key) {
        for (ctl::size_t i = 0; i < size; i++) {
            if (keys[i] == key) {
                return i;
            }
        }
        if (size < Cap) {
            return size++;
        }
        return -1;
    }

    // Get a key from the given cache index.
    V &get(CacheIndex idx) {
        ASSERT(idx < size);
        return values[idx];
    }
};
