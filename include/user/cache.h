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
            keys_[size_] = key;
            return size_++;
        }
        return -1;
    }
    CacheIndex getIdx(const K &key) const {
        for (Size i = 0; i < size_; i++) {
            if (keys_[i] == key) {
                return i;
            }
        }
        return -1;
    }

    // Get a key from the given cache index.
    V &get(CacheIndex idx) {
        ASSERT(idx < size_);
        return values_[idx];
    }
    const V &get(CacheIndex idx) const {
        ASSERT(idx < size_);
        return values_[idx];
    }

    V &get(const K &key) {
        return get(getIdx(key));
    }
    const V &get(const K &key) const {
        return get(getIdx(key));
    }

    K &getKey(CacheIndex idx) {
        ASSERT(idx < size_);
        return keys_[idx];
    }
    const K &getKey(CacheIndex idx) const {
        ASSERT(idx < size_);
        return keys_[idx];
    }
};
