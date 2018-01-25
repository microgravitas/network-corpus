#include <assert.h>
#include "bitset.h"

bitset_t *bitset_new(bitset_index_t max)
{
    bitset_t *set = (bitset_t *) malloc(sizeof(bitset_t));
    if(set == NULL)
    {
        fprintf(stderr, "[bitset] Error: could not allocate memory to store bitset\n");
        exit(0);
    }
    
    bitset_init(set, max);
    return set;
}

void bitset_init(bitset_t *set, bitset_index_t max)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

    set->max = max;
    set->bits = (bitset_data_t *) malloc(sizeof(bitset_data_t) * BITSET_NUM_DATA_ELEMENTS(max));
    if(set->bits == NULL)
    {
        fprintf(stderr, "[bitset] Error: could not allocate memory to store bitset data\n");
        exit(0);
    }
    
    bitset_clear_all(set);
}

void bitset_init_copy(bitset_t *set, const bitset_t const *source)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(source);
#endif

    bitset_init(set, source->max);
    bitset_copy(set, source);
}

void bitset_copy(bitset_t *set, const bitset_t const *source)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(source);
#endif
    
#ifdef BITSET_BOUNDS_CHECKING
    if(set->max != source->max)
    {
        fprintf(stderr, "[bitset] Error: out of bounds 1\n");
        exit(0);
    }
#endif

    memcpy(set->bits, source->bits, BITSET_NUM_DATA_ELEMENTS(set->max) * sizeof(bitset_data_t));
}

void bitset_destroy(bitset_t *set)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

    if(set->bits)
        free(set->bits);
    set->bits = NULL;
}

void bitset_free(bitset_t *set)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

    bitset_destroy(set);
    if(set)
        free(set);
}

void bitset_set(bitset_t *set, bitset_index_t index)
{
    bitset_index_t byte = index / BITSET_BITS_PER_ELEMENT;
    bitset_index_t bit = index % BITSET_BITS_PER_ELEMENT;

#ifdef BITSET_ASSERTIONS
    assert(set);
#endif
    
#ifdef BITSET_BOUNDS_CHECKING
    if(set->max < index)
    {
        fprintf(stderr, "[bitset] Error: index out of bounds 2\n");
        exit(0);
    }
#endif
    
    set->bits[byte] = set->bits[byte] | (bitset_data_t) ((bitset_data_t) 1 << (bitset_data_t) bit);
}

void bitset_set_all(bitset_t *set)
{
    bitset_index_t last_bit, last_element, i;
#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

    last_bit = set->max % BITSET_BITS_PER_ELEMENT;
    last_element = BITSET_NUM_DATA_ELEMENTS(set->max);
    for(i = 0; i < last_element; i++)
        set->bits[i] = set->bits[i] | ~set->bits[i];
    
    if(last_bit > 0)
        set->bits[last_element - 1] = set->bits[last_element - 1] & (((bitset_data_t) 1 << (bitset_data_t) (last_bit)) - 1);
}

void bitset_clear(bitset_t *set, bitset_index_t index)
{
    bitset_index_t byte = index / BITSET_BITS_PER_ELEMENT;
    bitset_index_t bit = index % BITSET_BITS_PER_ELEMENT;

#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(set->max < index)
    {
        fprintf(stderr, "[bitset] Error: index out of bounds 3\n");
        exit(0);
    }
#endif

    set->bits[byte] = set->bits[byte] & (bitset_data_t) ~((bitset_data_t) 1 << (bitset_data_t) bit);
}

void bitset_clear_all(bitset_t *set)
{
#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

    memset(set->bits, 0, BITSET_NUM_DATA_ELEMENTS(set->max) * sizeof(bitset_data_t));
}

bool bitset_get(const bitset_t const *set, const bitset_index_t index)
{
    bitset_index_t byte = index / BITSET_BITS_PER_ELEMENT;
    bitset_index_t bit = index % BITSET_BITS_PER_ELEMENT;

#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(set->max < index)
    {
        fprintf(stderr, "[bitset] Error: index out of bounds 4\n");
        exit(0);
    }
#endif
    
    return ((set->bits[byte] & (bitset_data_t) ((bitset_data_t) 1 << (bitset_data_t) bit)) > 0);
}

void bitset_toggle(bitset_t *set, bitset_index_t index)
{
    bitset_index_t byte = index / BITSET_BITS_PER_ELEMENT;
    bitset_index_t bit = index % BITSET_BITS_PER_ELEMENT;

#ifdef BITSET_ASSERTIONS
    assert(set);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(set->max < index)
    {
        fprintf(stderr, "[bitset] Error: index out of bounds 5\n");
        exit(0);
    }
#endif

    set->bits[byte] = set->bits[byte] ^ (bitset_data_t) ((bitset_data_t) 1 << (bitset_data_t) bit);
}

bool bitset_find_set_bit(const bitset_t const *set, bitset_index_t *result_bit)
{
    bitset_index_t byte, max_byte, bit = 0;
    bitset_data_t data;

#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(result_bit);
#endif

    max_byte = BITSET_NUM_DATA_ELEMENTS(set->max);
    for(byte = 0; byte < max_byte; byte++)
    {
        if(set->bits[byte] > 0)
        {
            data = set->bits[byte];
            while((data & 1) == 0)
            {
                data = data >> 1;
                bit++;
            }
            
            *result_bit = byte * BITSET_BITS_PER_ELEMENT + bit;
            return true;
        }
    }
    
    return false;
}

// TODO: Create a version of the function without the last argument, which must
// be set to null to go through the whole thing.
bool bitset_iterate_set_and_clear(bitset_t *set, bitset_index_t *next_bit, bitset_index_t *last_bit)
{
    bitset_index_t byte = 0, max_byte, bit = 0;
    bitset_data_t data;

#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(next_bit);
#endif
    
    max_byte = BITSET_NUM_DATA_ELEMENTS(set->max);
    
    if(last_bit)
        byte = *last_bit / BITSET_BITS_PER_ELEMENT;

    for(; byte < max_byte; byte++)
    {
        if(set->bits[byte] > 0)
        {
            data = set->bits[byte];
            
            while((data & 1) == 0)
            {
                data = data >> 1;
                bit++;
            }
            
            set->bits[byte] = set->bits[byte] & (bitset_data_t) ~((bitset_data_t) 1 << (bitset_data_t) bit);
            *next_bit = byte * BITSET_BITS_PER_ELEMENT + bit;
            
            if(last_bit)
                *last_bit = *next_bit;
            
            return true;
        }
    }
    
    return false;
}

void bitset_print(const bitset_t const *set)
{
    bitset_index_t i;
    bool append = false;
    
    printf("[");
    for(i = 0; i < set->max; i++)
    {
        if(!bitset_get(set, i))
            continue;
        
        if(append)
            printf(", %u", i);
        else
        {
            append = true;
            printf("%u", i);
        }
    }
    
    printf("]\n");
}

void bitset_remove_set(bitset_t *set, const bitset_t const *remove_set)
{
    bitset_index_t last_element, i;
    
#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(remove_set);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(set->max != remove_set->max)
    {
        fprintf(stderr, "Error: comparing bitsets of different size!\n");
        exit(0);
    }
#endif

    last_element = BITSET_NUM_DATA_ELEMENTS(set->max);
    for(i = 0; i < last_element; i++)
        set->bits[i] = set->bits[i] & ~remove_set->bits[i];
}

bool bitset_contains_set(const bitset_t const *set, const bitset_t const *subset)
{
    bitset_index_t i, last_element;

#ifdef BITSET_ASSERTIONS
    assert(set);
    assert(subset);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(set->max != subset->max)
    {
        fprintf(stderr, "Error: checking containment of sets of different size!\n");
        exit(0);
    }
#endif

    last_element = BITSET_NUM_DATA_ELEMENTS(set->max);
    for(i = 0; i < last_element; i++)
    {
        /* check if (set & subset) == subset */
        if((set->bits[i] & subset->bits[i]) != subset->bits[i])
            return false;
    }
    
    return true;
}

int bitset_cmp(const bitset_t const *a, const bitset_t const *b)
{
#ifdef BITSET_ASSERTIONS
    assert(a);
    assert(b);
#endif

#ifdef BITSET_BOUNDS_CHECKING
    if(a->max != b->max)
    {
        fprintf(stderr, "Error: comparing bitsets of different size!\n");
        exit(0);
    }
#endif

    return memcmp(a->bits, b->bits, BITSET_NUM_DATA_ELEMENTS(a->max) * sizeof(bitset_data_t));
}

