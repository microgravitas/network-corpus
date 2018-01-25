#include <assert.h>
#include "stack.h"

void stack_init(stack_t *stack, uint32_t size)
{
    assert(stack);
    assert(size > 0);
    
    stack->n = 0;
    stack->size = size;
    stack->allocated = 10;
    stack->data = (uint8_t *) malloc(sizeof(uint8_t) * stack->allocated * stack->size);
    assert(stack->data);
}

void stack_destroy(stack_t *stack)
{
    assert(stack);
    if(stack->data)
        free(stack->data);
    stack->data = NULL;
}

void stack_push(stack_t *stack, void *data)
{
    assert(stack);
    assert(data);
    
    if(stack->n == stack->allocated)
    {
        uint8_t *new_data = (uint8_t *) realloc(stack->data, 2 * sizeof(uint8_t) * stack->size * stack->allocated);
        if(new_data == NULL)
            return;

        stack->data = new_data;
        stack->allocated = stack->allocated * 2;
    }
    
    memcpy(stack->data + stack->n * stack->size, data, stack->size);
    stack->n = stack->n + 1;
}

bool stack_isempty(const stack_t const *stack)
{
    assert(stack);
    return (stack->n == 0);
}

uint32_t stack_height(const stack_t const *stack)
{
    //assert(stack);
    return stack->n;
}

bool stack_pop(stack_t *stack, void *data)
{
    assert(stack);
    assert(data);
    
    if(stack->n == 0)
        return false;
    
    memcpy(data, stack->data + (stack->n - 1) * stack->size, stack->size);
    stack->n = stack->n - 1;
    
    /* TODO: release space if load too small */
    return true;
}

bool stack_top(stack_t *stack, void *data)
{
    assert(stack);
    assert(data);
    
    if(stack->n == 0)
        return false;
    
    memcpy(data, stack->data + (stack->n - 1) * stack->size, stack->size);
    return true;
}

void stack_sort(stack_t *stack, int (*compar)(const void *, const void *))
{
    assert(stack);
    assert(compar);
    
    qsort(stack->data, stack->n, stack->size, compar);
}

bool stack_binsearch(stack_t *stack, int (*compar)(const void *, const void *), const void *data, uint32_t *position)
{
    uint32_t l, r, m;
    int res;
    
    assert(stack);
    assert(compar);
    
    l = 0;
    r = stack_height(stack);
    
    while(l < r)
    {
        m = (l + r) / 2;
        res = compar(stack->data + m * stack->size, data);
        
        if(res < 0)
            l = m + 1;
        else if(res > 0)
            r = m;
        else
        {
            if(position)
                *position = m;
            return true;
        }
    }

    return false;
}

bool stack_find_max(stack_t *stack, int (*compar)(const void *, const void *), void *result)
{
    uint32_t i, height;
    
    assert(stack);
    assert(compar);
    assert(result);
    
    height = stack_height(stack);
    if(height == 0)
        return false;
    
    memcpy(result, stack->data, stack->size);
    for(i = 1; i < height; i++)
    {
        if(compar(result, stack->data + i * stack->size) < 0)
            memcpy(result, stack->data + i * stack->size, stack->size);
    }

    return true;
}

bool stack_get_element(stack_t *stack, uint32_t position, void *result)
{
    uint32_t height;
    
    assert(stack);
    assert(result);
    
    height = stack_height(stack);
    if(height == 0)
        return false;
    else if(position >= height)
        return false;
    
    memcpy(result, stack->data + position * stack->size, stack->size);
    return true;
}

void stack_remove_element(stack_t *stack, int (*compar)(const void *, const void *), void *data)
{
    uint32_t i;
    void *tmp_data;
    assert(stack);
    assert(compar);
    assert(data);
    
    tmp_data = malloc(stack->size);
    if(tmp_data == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store temp stack data\n");
        exit(0);
    }
    
    i = 0;
    while(i < stack_height(stack))
    {
        stack_get_element(stack, i, tmp_data);
        if(compar(tmp_data, data) != 0)
            i++;
        else
        {
            /* put last element into this spot */
            stack_pop(stack, tmp_data);
            if(i < stack_height(stack))
                memcpy(stack->data + i * stack->size, tmp_data, stack->size);
        }
    }
    
    free(tmp_data);
}

bool stack_contains(stack_t *stack, int (*compar)(const void *, const void *), void *data)
{
    uint32_t i, height;
    
    assert(stack);
    assert(compar);
    assert(data);
    
    height = stack_height(stack);
    for(i = 0; i < height; i++)
    {
        if(compar(data, stack->data + i * stack->size) == 0)
            return true;
    }
    
    return false;
}

void *stack_get_element_ptr(const stack_t const *stack, uint32_t position)
{
    return &(stack->data[position * stack->size]);
}

