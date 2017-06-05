// Contains an implementation of a heap.

#pragma once
#include <def.h>
#include <std.h>
#include <types.h>
#include <swap.h>

namespace ctl {
template <size_t MaxSize, typename Data, typename Comp>
class Heap {
    Comp comp;
    Data entries[MaxSize];
    ctl::size_t size = 0;

    void bubbleDown(size_t place) {
        while (place < size) {
            auto left = place*2 + 1,
                 right = left + 1,
                 compWith = left;

            if (left >= size) return;

            if (right < size) {
                if (comp(entries[right], entries[left]))
                    compWith = right;
            }

            if (comp(entries[compWith], entries[place])) {
                swap(entries[compWith], entries[place]);
                place = compWith;
            }
            else return;
        }
    }

    void bubbleUp(size_t place) {
        while (place > 0) {
            auto parent = (place-1)/2;
            if (comp(entries[place], entries[parent])) {
                swap(entries[place], entries[parent]);
                place = parent;
            }
            else return;
        }
    }
public:
    const Data& peek() const {
        ASSERT(size >= 0);
        return entries[0];
    }

    const Data& pop() {
        ASSERT(--size >= 0);
        auto &root = entries[0];
        auto &last = entries[size];
        swap(root, last);
        bubbleDown(0);
        
        return last;
    }

    void push(Data data) {
        ASSERT(++size <= MaxSize);
        entries[size - 1] = data;

        bubbleUp(size - 1);
    }

    bool empty() const {
        return size == 0;
    }
};
}
