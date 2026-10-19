#include "stack.h"

#include <stdlib.h>

bool stack_init(Stack *stack, int initial_capacity) {
    stack->items = malloc((size_t)initial_capacity * sizeof(int));
    stack->capacity = initial_capacity;
    stack->size = 0;
    return stack->items != NULL;
}

void stack_free(Stack *stack) {
    free(stack->items);
    stack->items = NULL;
    stack->capacity = 0;
    stack->size = 0;
}

bool stack_is_empty(const Stack *stack) {
    return stack->size == 0;
}

/* Dobra a capacidade quando enche, mantendo o custo amortizado de cada push constante. */
bool stack_push(Stack *stack, int value) {
    if (stack->size == stack->capacity) {
        int new_capacity = stack->capacity * 2;
        int *grown = realloc(stack->items, (size_t)new_capacity * sizeof(int));
        if (grown == NULL) {
            return false;
        }
        stack->items = grown;
        stack->capacity = new_capacity;
    }
    stack->items[stack->size++] = value;
    return true;
}

/* Só pode ser chamada com a pilha não vazia. */
int stack_pop(Stack *stack) {
    return stack->items[--stack->size];
}
