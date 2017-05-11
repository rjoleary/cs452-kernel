#include "minheap.h"
#include "swap.h"

#define MINHEAP_ROOT 0

void minheapSetup(struct Minheap *heap) {
	heap->time = heap->size = 0;
}

static int getParentPlace(struct Minheap *heap, int place) {
	if (place <= 0) return -1;
	return (place - 1)/2;
}

static struct MinheapEntry* getParent(struct Minheap *heap, int place) {
	place = getParentPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

static int getLeftPlace(struct Minheap *heap, int place) {
	place = place*2 + 1;
	if (place >= heap->size) return -1;
	return place;
}

static struct MinheapEntry* getLeft(struct Minheap *heap, int place) {
	place = getLeftPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

static int getRightPlace(struct Minheap *heap, int place) {
	place = place*2 + 2;
	if (place >= heap->size) return -1;
	return place;
}

static struct MinheapEntry* getRight(struct Minheap *heap, int place) {
	place = getRightPlace(heap, place);
	if (place == -1) return 0;
	return &heap->entries[place];
}

// Return 0 for lhs < rhs, 1 for lhs > rhs
static int compareEntry(struct MinheapEntry *lhs, struct MinheapEntry *rhs) {
	if (lhs->data.priority == rhs->data.priority) return lhs->entryTime > rhs->entryTime;
	return lhs->data.priority > rhs->data.priority;
}

static void swapEntry(struct MinheapEntry *lhs, struct MinheapEntry *rhs) {
	swapInt(&lhs->data.id, &rhs->data.id);
	swapInt(&lhs->data.priority, &rhs->data.priority);
	swapInt(&lhs->entryTime, &rhs->entryTime);
}

static void bubbleUp(struct Minheap *heap, int place) {
	struct MinheapEntry *parent, *entry = &heap->entries[place];
	while ((parent = getParent(heap, place))
			&& compareEntry(parent, entry)) {
		swapEntry(parent, entry);
		entry = parent;
		place = getParentPlace(heap, place);
	}
}

int minheapPush(struct Minheap *heap, Tid id, Priority priority) {
	if (heap->size == NUM_TD) return -1;
	int place = heap->size;
	struct MinheapEntry *entry = &heap->entries[place];
	entry->data.id = id;
	entry->data.priority = priority;
	entry->entryTime = heap->time;
	heap->size++;
	heap->time++;

	bubbleUp(heap, place);

	return 0;
}

static void bubbleDown(struct Minheap *heap, int place) {
	while (1) {
		struct MinheapEntry *entry = &heap->entries[place],
		*left = getLeft(heap, place),
		*right = getRight(heap, place);
		int compare;
		if (left && right) compare = compareEntry(left, right);
		else if (left) compare = 0;
		else if (right) compare = 1;
		else return;

		switch(compare) {
		case 0:
			if (compareEntry(entry, left)) {
				swapEntry(entry, left);
				place = getLeftPlace(heap, place);
			}
			else return;
			break;
		case 1:
			if (compareEntry(entry, right)) {
				swapEntry(entry, right);
				place = getRightPlace(heap, place);
			}
			else return;
			break;
		}
	}
}

int minheapPop(struct Minheap *heap, struct MinheapData *data) {
	if (heap->size == 0) return -1;
	heap->size--;

	struct MinheapEntry *root = &heap->entries[MINHEAP_ROOT];
	*data = root->data;

	struct MinheapEntry *lastEntry = &heap->entries[heap->size];

	swapEntry(lastEntry, root);

	bubbleDown(heap, 0);

	return 0;
}
