#include "stack.h"
#include <stdlib.h>

void initStack(Stack * pstack, const unsigned int stackMaxCount, const unsigned int memberSize) {
    Stack stack = createStack(stackMaxCount, memberSize);
    memcpy(pstack, &stack, sizeof(stack));
}

Stack createStack(const unsigned int stackMaxCount, const unsigned int memberSize) {
    Stack stack = {
        .stackMaxCount = stackMaxCount,
        .memberSize = memberSize,
        .buffer = malloc(memberSize * stackMaxCount),
        .top = -1
    };
    return stack;
}

void destroyStack(Stack * pstack) {
    free(pstack->buffer);
    pstack->buffer = NULL;
}

bool isEmpty(const Stack stack) {
    return (stack.top < 0);
}

bool isFull(const Stack stack) {
    return (stack.top == (stack.stackMaxCount - 1));
}

void * peek(const Stack stack) {
    if (isEmpty(stack)){
        return NULL;
    }
    return (void *)((long)stack.buffer + (stack.memberSize * stack.top));
}

void * pop( Stack * pstack) {
    if (isEmpty(*pstack)){
        return NULL;
    }
    void * value = (void *)((long)pstack->buffer + (pstack->memberSize * pstack->top));
    pstack->top--;
    return value;
}

bool push(Stack * pstack, const void * pvalue ) {
    if (isFull(*pstack)) {
        return false;
    }
    pstack->top++;
    char * current = (char *)((long)pstack->buffer + (pstack->memberSize * pstack->top));
    for (int i = 0; i < pstack->memberSize; i++) {
        current[i] = ((char *)pvalue)[i];
    }
    return true;
}