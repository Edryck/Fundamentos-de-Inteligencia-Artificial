#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct {
    int *items;
    int  capacity;
    int  head;
    int  size;
} Queue;

bool queue_init(Queue *queue, int capacity);
void queue_free(Queue *queue);
bool queue_is_empty(const Queue *queue);
bool queue_push(Queue *queue, int value);
int  queue_pop(Queue *queue);

#endif
