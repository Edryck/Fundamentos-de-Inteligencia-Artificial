#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <stdbool.h>

typedef struct {
    int  node;
    long key;
    long tie_break;
    long order;
} HeapItem;

typedef struct {
    HeapItem *items;
    int       size;
    int       capacity;
    long      next_order;
} MinHeap;

bool min_heap_init(MinHeap *heap, int initial_capacity);
void min_heap_free(MinHeap *heap);
bool min_heap_is_empty(const MinHeap *heap);
bool min_heap_push(MinHeap *heap, int node, long key, long tie_break);
HeapItem min_heap_pop(MinHeap *heap);

#endif
