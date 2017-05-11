#include "minheap.h"
#include "swap.h"

#define MINHEAP_ROOT 0

void MinheapSetup(struct Minheap *heap) {
	heap->time = heap->size = 0;
}

static int GetParentPlace(struct Minheap *heap, int place) {
	if (place <= 0) return -1;
	return (place - 1)/2;
}

static struct MinheapEntry* GetParent(struct Minheap *heap, int place) {
	place = GetParentPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

static int GetLeftPlace(struct Minheap *heap, int place) {
	place = place*2 + 1;
	if (place >= heap->size) return -1;
	return place;
}

static struct MinheapEntry* GetLeft(struct Minheap *heap, int place) {
	place = GetLeftPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

static int GetRightPlace(struct Minheap *heap, int place) {
	place = place*2 + 2;
	if (place >= heap->size) return -1;
	return place;
}

static struct MinheapEntry* GetRight(struct Minheap *heap, int place) {
	place = GetRightPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

// Return 0 for lhs < rhs, 1 for lhs > rhs
static int CompareEntry(struct MinheapEntry *lhs, struct MinheapEntry *rhs) {
	if (lhs->data.priority == rhs->data.priority) return lhs->entryTime > rhs->entryTime;
	return lhs->data.priority > rhs->data.priority;
}

static void SwapEntry(struct MinheapEntry *lhs, struct MinheapEntry *rhs) {
	SwapInt(&lhs->data.id, &rhs->data.id);
	SwapInt(&lhs->data.priority, &rhs->data.priority);
	SwapInt(&lhs->entryTime, &rhs->entryTime);
}

static void BubbleUp(struct Minheap *heap, int place) {
	struct MinheapEntry *parent, *entry = &heap->entries[place];
	while ((parent = GetParent(heap, place))
			&& CompareEntry(parent, entry)) {
		SwapEntry(parent, entry);
		entry = parent;
		place = GetParentPlace(heap, place);
	}
}

int MinheapPush(struct Minheap *heap, Tid id, Priority priority) {
	if (heap->size == NUM_TD) return -1;
	int place = heap->size;
	struct MinheapEntry *entry = &heap->entries[place];
	entry->data.id = id;
	entry->data.priority = priority;
	entry->entryTime = heap->time;
	heap->size++;
	heap->time++;

	BubbleUp(heap, place);

	return 0;
}

static void BubbleDown(struct Minheap *heap, int place) {
	while (1) {
		struct MinheapEntry *entry = &heap->entries[place],
		*left = GetLeft(heap, place),
		*right = GetRight(heap, place);
		int compare;
		if (left && right) compare = CompareEntry(left, right);
		else if (left) compare = 0;
		else if (right) compare = 1;
		else return;

		switch(compare) {
		case 0:
			if (CompareEntry(entry, left)) {
				SwapEntry(entry, left);
				place = GetLeftPlace(heap, place);
			}
			else return;
			break;
		case 1:
			if (CompareEntry(entry, right)) {
				SwapEntry(entry, right);
				place = GetRightPlace(heap, place);
			}
			else return;
			break;
		}
	}
}

int MinheapPop(struct Minheap *heap, struct MinheapData *data) {
	if (heap->size == 0) return -1;
	heap->size--;

	struct MinheapEntry *root = &heap->entries[MINHEAP_ROOT];
	*data = root->data;

	struct MinheapEntry *lastEntry = &heap->entries[heap->size];

	SwapEntry(lastEntry, root);

	BubbleDown(heap, 0);

	return 0;
}
