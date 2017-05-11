#ifndef MINHEAP_H__INCLUDED
#define MINHEAP_H__INCLUDED

#include <types.h>
#include <def.h>

struct MinheapData {
	Tid id;
	Priority priority;
};

struct MinheapEntry {
	struct MinheapData data;
	int entryTime;
};

struct Minheap {
	struct MinheapEntry entries[NUM_TD];
	int time, size;
};

void MinheapSetup(struct Minheap *heap);
int MinheapPush(struct Minheap *heap, Tid id, Priority priority);
int MinheapPop(struct Minheap *heap, struct MinheapData *data);
#endif //MINHEAP_H__INCLUDED
