// Contains an implementation of a minheap.
// TODO: This minheap is unused until kernel 2.

#ifndef MINHEAP_H__INCLUDED
#define MINHEAP_H__INCLUDED

#include <def.h>
#include <types.h>

struct MinheapData {
    Tid id;
    Priority priority;
};

struct MinheapEntry {
    struct MinheapData data;
    int entryTime;
};

struct Minheap {
    struct MinheapEntry entries[kernel::NUM_TD];
    int time, size;
};

void minheapSetup(struct Minheap *heap);
int minheapPush(struct Minheap *heap, Tid id, Priority priority);
int minheapPop(struct Minheap *heap, struct MinheapData *data);

#endif // MINHEAP_H__INCLUDED
