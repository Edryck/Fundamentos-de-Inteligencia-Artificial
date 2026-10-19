#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

typedef struct {
    int *items;
    int  capacity;
    int  size;
} Stack;

bool stack_init(Stack *stack, int initial_capacity);
void stack_free(Stack *stack);
bool stack_is_empty(const Stack *stack);
bool stack_push(Stack *stack, int value);
int  stack_pop(Stack *stack);

#endif
