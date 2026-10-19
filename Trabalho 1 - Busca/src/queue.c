#include "queue.h"

#include <stdlib.h>

bool queue_init(Queue *queue, int capacity) {
    queue->items = malloc((size_t)capacity * sizeof(int));
    queue->capacity = capacity;
    queue->head = 0;
    queue->size = 0;
    return queue->items != NULL;
}

void queue_free(Queue *queue) {
    free(queue->items);
    queue->items = NULL;
    queue->capacity = 0;
    queue->head = 0;
    queue->size = 0;
}

bool queue_is_empty(const Queue *queue) {
    return queue->size == 0;
}

/* A fila é circular com capacidade fixa: quem usa dimensiona pelo número
   máximo de elementos que podem estar enfileirados ao mesmo tempo. */
bool queue_push(Queue *queue, int value) {
    if (queue->size == queue->capacity) {
        return false;
    }
    int tail = (queue->head + queue->size) % queue->capacity;
    queue->items[tail] = value;
    queue->size++;
    return true;
}

/* Só pode ser chamada com a fila não vazia. */
int queue_pop(Queue *queue) {
    int value = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return value;
}
