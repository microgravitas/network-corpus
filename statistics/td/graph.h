#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include "stack.h"
#include "bitset.h"

typedef uint32_t vertex_t;
typedef uint32_t vertex_label_t;

struct _graph_edge_s
{
    vertex_label_t from, to;
};
typedef struct _graph_edge_s edge_t;

struct _graph_builder_s
{
    stack_t vertex_labels, edges;
    vertex_label_t max_vertex_label;
};
typedef struct _graph_builder_s gbuild_t;

struct _graph_s
{
    uint32_t n, m;
    vertex_label_t *labels;
    uint32_t *positions;
    vertex_t *edges;
};
typedef struct _graph_s graph_t;

struct _subgraph_s
{
    bitset_t vertices;
    uint32_t num_vertices, num_edges;
};
typedef struct _subgraph_s subgraph_t;

struct _separator_s
{
    bitset_t vertices;
    uint32_t num_vertices, greatest_component;
    stack_t components;
};
typedef struct _separator_s separator_t;

struct _graph_rtree_s
{
    vertex_t root;
    uint32_t num_children;
    struct _graph_rtree_s *parent;
    struct _graph_rtree_s **children;
};
typedef struct _graph_rtree_s rtree_t;

struct _dfs_state_s
{
    vertex_t vertex;
    uint32_t edge_start, edge_end;
};
typedef struct _dfs_state_s dfs_state_t;

struct _graph_simple_dynamic_edge_s
{
    uint32_t neighbor_id, weight;
    struct _graph_simple_dynamic_edge_s *next;
};
typedef struct _graph_simple_dynamic_edge_s sd_edge_t;

struct _graph_simple_dynamic_s
{
    uint32_t num_vertices;
    sd_edge_t **edges;
};
typedef struct _graph_simple_dynamic_s sd_graph_t;

#define GRAPH_NUM_VERTICES(G) ((G)->n)
#define GRAPH_NUM_EDGES(G) ((G)->m)

int graph_cmp_vertex_labels(const void *a, const void *b);
int graph_cmp_vertices(const void *a, const void *b);
int graph_cmp_edges(const void *a, const void *b);

void gbuild_init(gbuild_t *builder);
void gbuild_destroy(gbuild_t *builder);
void gbuild_add_vertex(gbuild_t *builder, vertex_label_t vertex_label);
void gbuild_add_edge(gbuild_t *builder, vertex_label_t from, vertex_label_t to);
uint32_t gbuild_num_vertices(gbuild_t *builder);
uint32_t gbuild_num_edges(gbuild_t *builder);
bool gbuild_is_trivial(gbuild_t *builder);

void gbuild_create_graph(gbuild_t *builder, graph_t *graph);
void graph_destroy(graph_t *graph);
void graph_print(const graph_t const *graph);
uint32_t graph_num_vertices(const graph_t const *graph);
uint32_t graph_num_edges(const graph_t const *graph);
uint32_t graph_degree(const graph_t const *graph, const vertex_t vertex);
bool graph_edge_iter(const graph_t const *graph, const vertex_t vertex, uint32_t *iter_start, uint32_t *iter_end);
bool graph_get_vertex_by_label(const graph_t const *graph, const vertex_label_t vertex_label, vertex_t *result);
vertex_t graph_get_edge(const graph_t const *graph, const uint32_t edge);
bool graph_is_connected(const graph_t const *graph);

void subgraph_init(subgraph_t *subgraph, const bitset_t const *vertices, const uint32_t num_vertices, const uint32_t num_edges);
void subgraph_destroy(subgraph_t *subgraph);
bool subgraph_is_tree(const subgraph_t const *subgraph);
bool subgraph_is_clique(const subgraph_t const *subgraph);
void subgraph_dfs(const graph_t const *graph, const subgraph_t const *subgraph);

void separator_init_copy(separator_t *separator, const separator_t const *source);
void separator_destroy(separator_t *separator);
void separator_destroy_simple(separator_t *separator);
void separator_destroy_components(separator_t *separator);
void separator_construct_components(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *separator);
void separator_construct_from_components(const graph_t const *graph, const subgraph_t const *subgraph, const bitset_t const *separated_subgraph, stack_t *separators);

rtree_t *rtree_alloc(void);
rtree_t *rtree_new(vertex_t root);
void rtree_free(rtree_t *tree);
void rtree_add_child(rtree_t *parent, rtree_t *child);
void rtree_remove_child(rtree_t *parent, rtree_t *child);
void rtree_print(rtree_t *tree);
uint32_t rtree_height(rtree_t *tree);

#endif

