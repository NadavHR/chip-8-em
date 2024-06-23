#include <stdbool.h>

typedef struct
{
    const unsigned int stackMaxCount;
    const unsigned int memberSize;
    const void * buffer; // the size of the buffer will be stackMaxCount*memberSize
    int top;
} Stack;

void initStack(Stack * pstack,
                 const unsigned int stackMaxCount, 
                 const unsigned int memberSize);
Stack createStack(const unsigned int stackMaxCount, 
                 const unsigned int memberSize);
void destroyStack(Stack * pstack);
bool isEmpty(const Stack stack);
bool isFull(const Stack stack);
void * peek(const Stack stack);
void * pop( Stack * pstack);
bool push(Stack * pstack, const void * pvalue ); // returns true if pushed successfully, else returns false