#ifndef STACK_H
#define STACK_H

struct stack
{
    void *data;

    size_t size;
    size_t capacity;
    size_t el_size;
};

void  stack_ctor    (stack *const stk, const size_t el_size);
void  stack_dtor    (stack *const stk);

bool  stack_empty   (stack *const stk);
void  stack_push    (stack *const stk, const void *push_val);
void *stack_pop     (stack *const stk);
void *stack_front   (stack *const stk);
void  stack_realloc (stack *const stk);

#endif //STACK_H