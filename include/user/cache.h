#pragma once

#include <std.h>

// Key-value cache
template <Size Cap, typename K, typename V>
class Cache {
    K keys_[Cap];
    V values_[Cap];
    Size size_;
public:
    typedef Size CacheIndex;

    // Return the number of cached values.
    Size size() const {
        return size_;
    }

    // Returns the cache index or -1 if full
    CacheIndex getIdx(const K &key) {
        for (Size i = 0; i < size_; i++) {
            if (keys_[i] == key) {
                return i;
            }
        }
        if (size_ < Cap) {
            return size_++;
        }
        return -1;
    }

    // Get a key from the given cache index.
    V &get(CacheIndex idx) {
        ASSERT(idx < size_);
        return values_[idx];
    }
};
