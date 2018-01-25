#ifndef UNION_FIND_H_INCLUDED
#define UNION_FIND_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @defgroup UnionFind Union-Find
 * @{
 */

/** @brief Type to store component parents. This type needs to be big enough to store all component ids. */
typedef uint32_t uf_node_t;

struct _union_find_s
{
    /** @brief Number of nodes this structure keeps track of. Valid IDs are in the range `[0, num_nodes - 1]`. */
    uf_node_t num_nodes;
    /** @brief Parents of nodes in the component tree */
    uf_node_t *parents;
};
typedef struct _union_find_s uf_t;

void uf_init(uf_t *uf, uf_node_t num_nodes);
void uf_destroy(uf_t *uf);
uf_node_t uf_find(uf_t *uf, uf_node_t node);
void uf_union(uf_t *uf, uf_node_t u, uf_node_t v);

/** @} */

#endif

