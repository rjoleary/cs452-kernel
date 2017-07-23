#pragma once

#include <std.h>

// Key-value map
// Once the map is full, it's full - no more values can be added.
template <Size Cap, typename K, typename V>
class FixedMap {
    K keys_[Cap];
    V values_[Cap];
    Size size_ = 0;
public:
    // Return the number of cached values.
    Size size() const {
        return size_;
    }

    // Return whether using the given key will overflow.
    bool willOverflow(const K &key) const {
        return !(has(key) || size_ < Cap);
    }

    // Return whether the given value is in the cache.
    bool has(const K &key) const {
        for (Size i = 0; i < size_; i++) {
            if (keys_[i] == key) {
                return true;
            }
        }
        return false;
    }

    // Return value given a key.
    V &get(const K &key) {
        for (Size i = 0; i < size_; i++) {
            if (keys_[i] == key) {
                return values_[i];
            }
        }
        ASSERT(size_ <= Cap);
        keys_[size_] = key;
        return values_[size_++];
    }
    const V &get(const K &key) const {
        for (Size i = 0; i < size_; i++) {
            if (keys_[i] == key) {
                return values_[i];
            }
        }
        ctl::assert("key not found");
    }

    // Iterator for keys
    struct Keys {
        K *begin_, *end_;
        K *begin() {
            return begin_;
        }
        K *end() {
            return end_;
        }
    };
    Keys keys() {
        return Keys{keys_, keys_ + size_};
    }

    // Const iterator for keys
    struct CKeys {
        const K *begin_, *end_;
        const K *begin() {
            return begin_;
        }
        const K *end() {
            return end_;
        }
    };
    CKeys keys() const {
        return CKeys{keys_, keys_ + size_};
    }

    // Iterator for values
    struct Values {
        V *begin_, *end_;
        V *begin() {
            return begin_;
        }
        V *end() {
            return end_;
        }
    };
    Values values() {
        return Values{values_, values_ + size_};
    }
};
