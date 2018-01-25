#ifndef BITSET_H_INCLUDED
#define BITSET_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//#define BITSET_BOUNDS_CHECKING
//#define BITSET_ASSERTIONS

typedef uint16_t bitset_index_t;
typedef uint8_t bitset_data_t;

struct _bitset_s
{
    bitset_index_t max;
    bitset_data_t *bits;
};
typedef struct _bitset_s bitset_t;

#define BITSET_NUM_DATA_ELEMENTS(num_bits) (((num_bits) + 8 * sizeof(bitset_data_t) - 1) / (8 * sizeof(bitset_data_t)))
#define BITSET_BITS_PER_ELEMENT (8 * sizeof(bitset_data_t))

bitset_t *bitset_new(bitset_index_t max);
void bitset_init(bitset_t *set, bitset_index_t max);
void bitset_init_copy(bitset_t *set, const bitset_t const *source);
void bitset_copy(bitset_t *set, const bitset_t const *source);
void bitset_destroy(bitset_t *set);
void bitset_free(bitset_t *set);
void bitset_set(bitset_t *set, bitset_index_t index);
void bitset_set_all(bitset_t *set);
void bitset_clear(bitset_t *set, bitset_index_t index);
void bitset_clear_all(bitset_t *set);
bool bitset_get(const bitset_t const *set, const bitset_index_t index);
void bitset_toggle(bitset_t *set, bitset_index_t index);
bool bitset_find_set_bit(const bitset_t const *set, bitset_index_t *bit);
bool bitset_iterate_set_and_clear(bitset_t *set, bitset_index_t *next_bit, bitset_index_t *last_bit);
void bitset_print(const bitset_t const *set);
void bitset_remove_set(bitset_t *set, const bitset_t const *remove_set);
bool bitset_contains_set(const bitset_t const *set, const bitset_t const *subset);
int bitset_cmp(const bitset_t const *a, const bitset_t const *b);

#endif

