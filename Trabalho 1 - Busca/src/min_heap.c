#include "min_heap.h"

#include <stdlib.h>

bool min_heap_init(MinHeap *heap, int initial_capacity) {
    heap->items = malloc((size_t)initial_capacity * sizeof(HeapItem));
    heap->size = 0;
    heap->capacity = initial_capacity;
    heap->next_order = 0;
    return heap->items != NULL;
}

void min_heap_free(MinHeap *heap) {
    free(heap->items);
    heap->items = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

bool min_heap_is_empty(const MinHeap *heap) {
    return heap->size == 0;
}

/* Desempata pela chave secundária e, por último, pela ordem de inserção. Sem isso a ordem
   de saída de itens iguais dependeria da forma do heap e a execução deixaria de ser reprodutível. */
static bool comes_before(const HeapItem *a, const HeapItem *b) {
    if (a->key != b->key) {
        return a->key < b->key;
    }
    if (a->tie_break != b->tie_break) {
        return a->tie_break < b->tie_break;
    }
    return a->order < b->order;
}

static void swap_items(HeapItem *a, HeapItem *b) {
    HeapItem temp = *a;
    *a = *b;
    *b = temp;
}

static void sift_up(MinHeap *heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (!comes_before(&heap->items[index], &heap->items[parent])) {
            break;
        }
        swap_items(&heap->items[index], &heap->items[parent]);
        index = parent;
    }
}

static void sift_down(MinHeap *heap, int index) {
    for (;;) {
        int smallest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        if (left < heap->size && comes_before(&heap->items[left], &heap->items[smallest])) {
            smallest = left;
        }
        if (right < heap->size && comes_before(&heap->items[right], &heap->items[smallest])) {
            smallest = right;
        }
        if (smallest == index) {
            return;
        }
        swap_items(&heap->items[index], &heap->items[smallest]);
        index = smallest;
    }
}

bool min_heap_push(MinHeap *heap, int node, long key, long tie_break) {
    if (heap->size == heap->capacity) {
        int new_capacity = heap->capacity * 2;
        HeapItem *grown = realloc(heap->items, (size_t)new_capacity * sizeof(HeapItem));
        if (grown == NULL) {
            return false;
        }
        heap->items = grown;
        heap->capacity = new_capacity;
    }
    heap->items[heap->size] = (HeapItem){node, key, tie_break, heap->next_order++};
    sift_up(heap, heap->size);
    heap->size++;
    return true;
}

/* Só pode ser chamada com o heap não vazio. */
HeapItem min_heap_pop(MinHeap *heap) {
    HeapItem top = heap->items[0];
    heap->size--;
    heap->items[0] = heap->items[heap->size];
    sift_down(heap, 0);
    return top;
}
