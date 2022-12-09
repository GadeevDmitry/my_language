#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "stack.h"
#include "../logs/log.h"

void stack_ctor(stack *const stk, const size_t el_size)
{
    assert(stk != nullptr);

   *stk = {};
    stk->el_size  = el_size;
    stk->data     = log_calloc(el_size, 4); //default elementary capacity
    stk->capacity = 4;
}

void stack_dtor(stack *const stk)
{
    assert(stk != nullptr);

    log_free(stk->data);
}

void stack_push(stack *const stk, const void *push_val)
{
    assert(stk      != nullptr);
    assert(push_val != nullptr);

    stack_realloc(stk);
    memcpy((char *) stk->data + stk->size * stk->el_size, push_val, stk->el_size);
    ++stk->size;
    stack_realloc(stk);
}

void *stack_pop(stack *const stk)
{
    assert(stk != nullptr);
    assert(stk->size > 0);

    void *ret = (char *) stk->data + (stk->size - 1) * stk->el_size;

    --stk->size;
    return ret;
}

void *stack_front(stack *const stk)
{
    assert(stk != nullptr);

    return (char *) stk->data + (stk->size - 1) * stk->el_size;
}

bool stack_empty(stack *const stk)
{
    assert(stk != nullptr);

    return stk->size == 0;
}

void stack_realloc(stack *const stk)
{
    assert(stk != nullptr);

    if (stk->size == stk->capacity)
    {
        stk->capacity *= 2;
        stk->data      = log_realloc(stk->data, stk->el_size * stk->capacity);
    }
}