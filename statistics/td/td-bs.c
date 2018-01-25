#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include "graph.h"
#include "union_find.h"
#include "kbtree.h"

#define MAX_INPUT_BUFFER_SIZE 1024
/*#define DEBUG_CHECK_MINIMALITY*/

struct _dfs_rtree_state_s
{
    rtree_t *vertex;
    uint32_t next_child;
};
typedef struct _dfs_rtree_state_s dfs_rtree_state_t;

struct _separator_score_s
{
    uint32_t size_maximum_component, num_components;
};
typedef struct _separator_score_s score_t;

bool cfg_show_comments = false;
bool cfg_separator_timeout = false;
bool cfg_single_local_search = false;
bool cfg_output_decomposition = false;

bool cfg_score_max_comp_size = false;
bool cfg_score_max_comp_num = false;
bool cfg_score_max_weight = false;

long long current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

int cmp_separator(const separator_t a, const separator_t b)
{
    return bitset_cmp(&(a.vertices), &(b.vertices));
}

KBTREE_INIT(separators, separator_t, cmp_separator)

int cmp_separator_ptr(const void *a, const void *b)
{
    const separator_t *u = (const separator_t *) a;
    const separator_t *v = (const separator_t *) b;
    return bitset_cmp(&(u->vertices), &(v->vertices));
}

bool load_dimacs_graph(gbuild_t *builder, const char *filename, const bool show_comments)
{
    FILE *fd;
    char buffer[MAX_INPUT_BUFFER_SIZE];
    size_t len;
    uint32_t n, m, last_edge = 0;
    bool problem_desc = false;
    
    fd = fopen(filename, "rb");
    if(fd == NULL)
    {
        fprintf(stderr, "Error: could not open input file for reading (%s)\n", strerror(errno));
        return false;
    }
    
    gbuild_init(builder);
    
    while(fgets(buffer, MAX_INPUT_BUFFER_SIZE, fd))
    {
        len = strlen(buffer);
        
        /* skip unimportant lines */
        if(len <= 1)
            continue;

        /* chomp off carriage-return-new-line */
        if(buffer[len - 1] == '\n')
            buffer[--len] = '\0';
        if(len && (buffer[len - 1] == '\r'))
            buffer[--len] = '\0';        
        
        if((buffer[0] == 'c') && show_comments)
        {
            /* c This is an example of a comment line. */
            fprintf(stdout, "Info: %s\n", buffer);
        }
        else if(buffer[0] == 'p')
        {
            /* p FORMAT NODES EDGES */
            if(problem_desc)
            {
                fprintf(stderr, "Error: multiple 'problem' descriptions in input file\n");
                gbuild_destroy(builder);
                fclose(fd);
                return false;
            }
            else
            {
                if(sscanf(buffer, "p edge %u %u", &n, &m) != 2)
                {
                    fprintf(stderr, "Error: could not parse 'problem' description in input file\n");
                    gbuild_destroy(builder);
                    fclose(fd);
                    return false;
                }
                else
                    problem_desc = true;
            }
        }
        else if(buffer[0] == 'e')
        {
            int u, v;
            /* e W V */
            if(sscanf(buffer, "e %i %i", &u, &v) != 2)
            {
                fprintf(stderr, "Error: could not parse 'edge' description\n");
                gbuild_destroy(builder);
                fclose(fd);
                return false;
            }
            else if((u <= 0) || (v <= 0) || (u == v))
            {
                fprintf(stderr, "Error: invalid range for vertex ids in 'edge' description\n");
                gbuild_destroy(builder);
                fclose(fd);
                return false;
            }
            else if(last_edge == m)
            {
                fprintf(stderr, "Error: too many edges in input file\n");
                gbuild_destroy(builder);
                fclose(fd);
                return false;
            }
            else
            {
                if(u > v)
                {
                    int t = v;
                    v = u;
                    u = t;
                }
                
                gbuild_add_edge(builder, u, v);
                last_edge++;
            }
        }
    }
    
    fclose(fd);
    return true;
}

uint32_t max_component_size(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *separator, bool compute_components)
{
    uint32_t i, max_size = 0;
    
    /* compute components that are separated */
    if(compute_components)
        separator_construct_components(graph, subgraph, separator);

    /* find biggest component */
    for(i = 0; i < stack_height(&(separator->components)); i++)
    {
        subgraph_t *component = stack_get_element_ptr(&(separator->components), i);
        if(max_size < component->num_vertices)
            max_size = component->num_vertices;
    }
    
    separator->greatest_component = max_size;
    return max_size;
}

uint32_t num_components(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *separator, bool compute_components)
{
    if(compute_components)
        separator_construct_components(graph, subgraph, separator);
    
    return stack_height(&(separator->components));
}

uint32_t compute_separator_score(separator_t *separator, const uint32_t *weights)
{
    if(cfg_score_max_comp_size)
        return separator->greatest_component;
    else if(cfg_score_max_weight)
    {
        uint32_t weight = 0;
        bitset_index_t next_vertex, last_vertex;
        bitset_t vertex_iter;
        
        bitset_init_copy(&vertex_iter, &(separator->vertices));
        last_vertex = 0;
        while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
        {
            weight = weight + weights[next_vertex];
        }
        
        bitset_destroy(&vertex_iter);
        
        return (UINT32_MAX - weight);
    }
    else
        return (UINT32_MAX - stack_height(&(separator->components)));
}

void improve_separator(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *separator, const uint32_t *weights)
{
    bool improved;
    uint32_t best_separator_score, best_separator_size;
    uint32_t edge_start, edge_end, iterations = 0;
    bitset_t vertex_iter, new_separated_subgraph;
    bitset_index_t last_vertex, next_vertex;
    stack_t tmp_separators, next_separators;
    separator_t current_separator;
    kbtree_t(separators) *finished_separators;
    
    if(separator->greatest_component == 0)
    {
        fprintf(stderr, "[debug] recomputing components\n");
        max_component_size(graph, subgraph, separator, true);
    }

    best_separator_score = compute_separator_score(separator, weights);
    best_separator_size = separator->num_vertices;
    
    stack_init(&next_separators, sizeof(separator_t));
    finished_separators = kb_init(separators, KB_DEFAULT_SIZE);
    
    bitset_init_copy(&(current_separator.vertices), &(separator->vertices));
    current_separator.num_vertices = separator->num_vertices;
    current_separator.greatest_component = 0;
    
    kb_put(separators, finished_separators, current_separator);
    
    do
    {
        improved = false;
        iterations++;
        
        //printf("best separator: size: %u, score: %u\n", best_separator_size, best_separator_score);
        /* foreach x in current_separator: remove (current_separator \cup N(x)) */
        bitset_init_copy(&vertex_iter, &(separator->vertices));
        last_vertex = 0;
        while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
        {
            /* copy subgraph and remove the separator */
            bitset_init_copy(&new_separated_subgraph, &(subgraph->vertices));
            bitset_remove_set(&new_separated_subgraph, &(separator->vertices));

            /* remove the neighborhood of next_vertex */
            graph_edge_iter(graph, next_vertex, &edge_start, &edge_end);
            for(; edge_start < edge_end; edge_start++)
                bitset_clear(&new_separated_subgraph, graph_get_edge(graph, edge_start));
            
            /* calculate S = N(C) for each component C */
            separator_construct_from_components(graph, subgraph, &new_separated_subgraph, &tmp_separators);
            
            /* go through list of separators */
            while(stack_pop(&tmp_separators, &current_separator))
            {
                if(kb_get(separators, finished_separators, current_separator))
                    separator_destroy_simple(&current_separator);
                else
                {
                    uint32_t separator_score;
                    
                    current_separator.greatest_component = 0;
                    kb_put(separators, finished_separators, current_separator);
                    
                    max_component_size(graph, subgraph, &current_separator, true);
                    separator_score = compute_separator_score(&current_separator, weights);
                    
                    if((separator_score < best_separator_score) || ((separator_score == best_separator_score) && (current_separator.num_vertices < best_separator_size)))
                        stack_push(&next_separators, &current_separator);
                    else
                        separator_destroy_components(&current_separator);
                }
            }

            stack_destroy(&tmp_separators);
            bitset_destroy(&new_separated_subgraph);
        }
        bitset_destroy(&vertex_iter);
        
        while(stack_pop(&next_separators, &current_separator))
        {
            uint32_t separator_score = compute_separator_score(&current_separator, weights);

            if((separator_score < best_separator_score) || ((separator_score == best_separator_score) && (current_separator.num_vertices < best_separator_size)))
            {
                separator_destroy(separator);
                separator_init_copy(separator, &current_separator);
                best_separator_score = separator_score;
                best_separator_size = separator->num_vertices;
                improved = true;
            }
            
            separator_destroy_components(&current_separator);
        }
    }
    while(improved);
    
    stack_destroy(&next_separators);
    __kb_traverse(separator_t, finished_separators, separator_destroy_simple);
    kb_destroy(separators, finished_separators);
}

uint32_t find_best_separator(const graph_t const *graph, const subgraph_t const *subgraph, stack_t *separators, separator_t *best_separator, const uint32_t *weights)
{
    separator_t current_separator;
    bool found = false;
    uint32_t best_separator_score = 0, best_separator_size = 0;
    
    while(stack_pop(separators, &current_separator))
    {
        uint32_t separator_score;

        max_component_size(graph, subgraph, &current_separator, true);
        separator_score = compute_separator_score(&current_separator, weights);
        
        if(!found || (separator_score < best_separator_score) || ((separator_score == best_separator_score) && (current_separator.num_vertices < best_separator_size)))
        {
            if(found)
                separator_destroy(best_separator);

            *best_separator = current_separator;
            found = true;
            best_separator_score = separator_score;
            best_separator_size = best_separator->num_vertices;
        }
        else
            separator_destroy(&current_separator);
    }
    
    return best_separator_size;
}

bool separator_is_minimal(const graph_t const *graph, separator_t *separator)
{
    uint32_t i;
    bitset_t vertex_iter;
    bitset_index_t next_vertex, last_vertex;
    bool is_minimal_separator = true;
    
    for(i = 0; i < stack_height(&(separator->components)); i++)
    {
        subgraph_t *component = stack_get_element_ptr(&(separator->components), i);
        
        /* test whether every vertex of the separator has a neighbor in this component */
        bitset_init_copy(&vertex_iter, &(separator->vertices));
        last_vertex = 0;
        while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
        {
            uint32_t edge_start, edge_end;
            bool has_neighbor = false;

            graph_edge_iter(graph, next_vertex, &edge_start, &edge_end);

            for(; edge_start < edge_end; edge_start++)
            {
                vertex_t neighbor = graph_get_edge(graph, edge_start);
                if(bitset_get(&(component->vertices), neighbor))
                {
                    has_neighbor = true;
                    break;
                }
            }
                    
            if(has_neighbor == false)
            {
                printf("vertex %u does not have a neighbor in some component\n", next_vertex);
                is_minimal_separator = false;
            }
        }
        bitset_destroy(&vertex_iter);
        
        if(is_minimal_separator == false)
            break;
    }
            
    return is_minimal_separator;
}

void find_close_separators(const graph_t const *graph, const subgraph_t const *subgraph, stack_t *new_separators)
{
    bitset_index_t next_vertex, last_vertex;
    bitset_t separated_subgraph, vertex_iter;
    uint32_t edge_start, edge_end;
    stack_t tmp_separators;
    separator_t current_separator;
    
    bitset_init_copy(&vertex_iter, &(subgraph->vertices));
    last_vertex = 0;
    while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
    {
        /* create new subgraph, copy old one and remove vertex and its neighborhood */
        bitset_init_copy(&separated_subgraph, &(subgraph->vertices));
        
        /* remove vertex next_vertex */
#ifdef DEBUG_CHECK_MINIMALITY
        printf("removing %u from graph and its neighborhood\n", graph->labels[next_vertex]);
#endif
        bitset_clear(&separated_subgraph, (bitset_index_t) next_vertex);
        
        /* iterate through neighborhood of next_vertex and remove all neighbors from the subgraph */
        graph_edge_iter(graph, next_vertex, &edge_start, &edge_end);
        for(; edge_start < edge_end; edge_start++)
        {
            vertex_t neighbor = graph_get_edge(graph, edge_start);
#ifdef DEBUG_CHECK_MINIMALITY
            printf("removing %u from graph\n", graph->labels[neighbor]);
#endif
            bitset_clear(&separated_subgraph, (bitset_index_t) neighbor);
        }

        /* calculate S = N(C) for each component C */
        separator_construct_from_components(graph, subgraph, &separated_subgraph, &tmp_separators);

#ifdef DEBUG_CHECK_MINIMALITY
        printf("%u components\n", stack_height(&tmp_separators));
#endif

        /* go through list of separators */
        while(stack_pop(&tmp_separators, &current_separator))
        {
#ifdef DEBUG_CHECK_MINIMALITY
            separator_construct_components(graph, subgraph, &current_separator);
            if(!separator_is_minimal(graph, &current_separator))
            {
                fprintf(stderr, "[error] close separator is not minimal\n");
                exit(0);
            }
            separator_destroy_components(&current_separator);
#endif
            stack_push(new_separators, &current_separator);
        }

#ifdef DEBUG_CHECK_MINIMALITY
        printf("all close separators of %u are minimal\n", next_vertex);
#endif

        bitset_destroy(&separated_subgraph);
        stack_destroy(&tmp_separators);
    }
    bitset_destroy(&vertex_iter);
}

bool find_balanced_separator_local_search(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *best_separator, const uint32_t *weights)
{
    uint32_t best_separator_size = 0, best_separator_score = 0, separator_score;
    stack_t close_separators;
    separator_t current_separator;
    
    /* find separators that are close to some vertex */
    stack_init(&close_separators, sizeof(separator_t));
    find_close_separators(graph, subgraph, &close_separators);
    
    /* no separators means we have a clique or just two vertices */
    if(stack_height(&close_separators) == 0)
    {
        stack_destroy(&close_separators);
        return false;
    }
    
    if(cfg_single_local_search)
    {
        best_separator_size = find_best_separator(graph, subgraph, &close_separators, best_separator, weights);
        improve_separator(graph, subgraph, best_separator, weights);
    }
    else
    {
        /* perform improve_separator for every close separator */
        while(stack_pop(&close_separators, &current_separator))
        {
            /* first compute components, then improve */
            max_component_size(graph, subgraph, &current_separator, true);
            improve_separator(graph, subgraph, &current_separator, weights);
            
            separator_score = compute_separator_score(&current_separator, weights);
            if((best_separator_size == 0) || (separator_score < best_separator_score) || ((separator_score == best_separator_score) && (current_separator.num_vertices < best_separator_size)))
            {
                if(best_separator_size > 0)
                    separator_destroy(best_separator);

                *best_separator = current_separator;
                best_separator_size = best_separator->num_vertices;
                best_separator_score = separator_score;
            }
            else
                separator_destroy(&current_separator);
        }
    }
    
    stack_destroy(&close_separators);
    
    return true;
}

rtree_t *tree_create_chain(const bitset_t const *vertices)
{
    bitset_index_t last_vertex, next_vertex;
    bitset_t vertex_iter;
    rtree_t *tree = NULL;
    
    bitset_init_copy(&vertex_iter, vertices);
    last_vertex = 0;
    while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
    {
		// TODO: Does the order in this loop make sense?
        rtree_t *tree_child = rtree_new(next_vertex);
        if(tree)
            rtree_add_child(tree_child, tree);
        
        tree = tree_child;
    }
    
    bitset_destroy(&vertex_iter);
    
    return tree;
}

void print_decomposition(const graph_t const *graph, const char const *filename, rtree_t *tree)
{
    stack_t dfs_stack;
    FILE *fd = fopen(filename, "ab");

    if(fd == NULL)
    {
        fprintf(stderr, "Error: could not open '%s' for appending the treedepth decomposition\n", filename);
        return;
    }
    
    fprintf(fd, "c Treedepth decomposition generated by td-balanced-separators, height: %u\n", rtree_height(tree));
    
    stack_init(&dfs_stack, sizeof(rtree_t *));
    
    stack_push(&dfs_stack, &tree);
    
    while(stack_pop(&dfs_stack, &tree))
    {
        uint32_t i;
        
        if(tree->parent)
            fprintf(fd, "t %u %u\n", graph->labels[tree->parent->root], graph->labels[tree->root]);
        
        for(i = 0; i < tree->num_children; i++)
            stack_push(&dfs_stack, &(tree->children[i]));
    }
    
    stack_destroy(&dfs_stack);
    fclose(fd);
}

uint32_t td_balanced_sep(const graph_t *graph, const subgraph_t const *subgraph, rtree_t **tree, uint32_t *weights)
{
    bool separator_found;
    uint32_t i, depth;
    separator_t best_separator;
    subgraph_t *component;
    
    *tree = NULL;
    
    separator_found = find_balanced_separator_local_search(graph, subgraph, &best_separator, weights);

    if(!separator_found)
    {
        *tree = tree_create_chain(&(subgraph->vertices));
        depth = subgraph->num_vertices;
    }
    else
    {
        uint32_t max_depth = 0;
        rtree_t *component_parent;
        
        *tree = tree_create_chain(&(best_separator.vertices));
        component_parent = *tree;
        
        while(component_parent->num_children)
            component_parent = component_parent->children[0];
        
        if(weights)
        {
            bitset_t vertex_iter;
            bitset_index_t next_vertex, last_vertex;
            
            bitset_init_copy(&vertex_iter, &(best_separator.vertices));
            last_vertex = 0;
            while(bitset_iterate_set_and_clear(&vertex_iter, &next_vertex, &last_vertex))
            {
                uint32_t edge_start, edge_end;
                graph_edge_iter(graph, next_vertex, &edge_start, &edge_end);
                
                for(; edge_start < edge_end; edge_start++)
                {
                    vertex_t neighbor = graph_get_edge(graph, edge_start);
                    weights[neighbor] = weights[neighbor] + 1;
                }
            }
            
            bitset_destroy(&vertex_iter);
        }
        
        for(i = 0; i < stack_height(&(best_separator.components)); i++)
        {
            rtree_t *component_tree = NULL;
            
            component = stack_get_element_ptr(&(best_separator.components), i);
            depth = td_balanced_sep(graph, component, &component_tree, weights);
            
            if(component_tree)
                rtree_add_child(component_parent, component_tree);
            else
            {
                fprintf(stderr, "[error] td_balanced_sep(): no tree returned for component\n");
                exit(0);
            }
            
            if(max_depth < depth)
                max_depth = depth;
        }

        depth = max_depth + best_separator.num_vertices;
            
        separator_destroy(&best_separator);
    }
    
    return depth;
}

uint32_t make_nice(const graph_t const *graph, const subgraph_t const *subgraph, rtree_t **tree)
{
    stack_t node_ordering;
    stack_t dfs_stack;
    rtree_t *node;
    uf_t components;
    vertex_t vertex;
    uint32_t i;
    uint32_t edge_start, edge_end;
    uf_node_t comp_node, comp_node_child;
    rtree_t **component_parents;

    stack_init(&node_ordering, sizeof(vertex_t));
    stack_init(&dfs_stack, sizeof(rtree_t *));

    node = *tree;
    stack_push(&dfs_stack, &node);

    while(stack_pop(&dfs_stack, &node))
    {
        stack_push(&node_ordering, &(node->root));
        for(i = 0; i < node->num_children; i++)
        {
            stack_push(&dfs_stack, &(node->children[i]));
        }
    }

    rtree_free(*tree);
    
    uf_init(&components, graph_num_vertices(graph));
    component_parents = (rtree_t **) malloc(sizeof(rtree_t *) * graph_num_vertices(graph));
    for(i = 0; i < graph_num_vertices(graph); i++)
    {
        component_parents[i] = NULL;
    }
    
    while(stack_pop(&node_ordering, &vertex))
    {
        /* adding 'vertex' to the tree */
        node = rtree_new(vertex);
        comp_node = uf_find(&components, (uf_node_t) vertex);
        component_parents[comp_node] = node;
        
        graph_edge_iter(graph, vertex, &edge_start, &edge_end);
        for(; edge_start < edge_end; edge_start++)
        {
            vertex_t neighbor = graph_get_edge(graph, edge_start);

            if(component_parents[neighbor] == NULL)
                continue;

            if(bitset_get(&(subgraph->vertices), neighbor) == false)
                continue;

            comp_node = uf_find(&components, vertex);
            comp_node_child = uf_find(&components, neighbor);

            if(comp_node == comp_node_child)
                continue;

            /* join these components by turning 'comp_node' into the new root */
            rtree_add_child(component_parents[comp_node], component_parents[comp_node_child]);
            node = component_parents[comp_node];
            uf_union(&components, vertex, neighbor);
            comp_node = uf_find(&components, vertex);
            component_parents[comp_node] = node;
        }
        
        *tree = node;
    }
    
    uf_destroy(&components);
    free(component_parents);
    stack_destroy(&node_ordering);
    stack_destroy(&dfs_stack);

    return rtree_height(*tree);
}

int main(int argc, char **argv)
{
    gbuild_t builder;
    graph_t graph;
    subgraph_t full_graph;
    bitset_t vertices;
    uint32_t td, i, *weights = NULL;
    long long time_start, time_end;
    rtree_t *tree;
    
    if(argc < 2)
    {
        fprintf(stdout, "Usage: %s [<options>] <graph.dimacs>\n", argv[0]);
        fprintf(stdout, "Options:\n");
        fprintf(stdout, "     -c  --show-comments        shows comments from input file\n");
        fprintf(stdout, "     -s  --single-local-search  only performs local search from one close separator.\n");
        fprintf(stdout, "     -m  --max-component        try to minimize size of greatest component\n");
        fprintf(stdout, "     -n  --num-components       try to maximize number of component\n");
        fprintf(stdout, "     -w  --weights              try to maximize weight of separator\n");
        fprintf(stdout, "     -p  --print-decomposition  Append treedepth decomposition to file.\n");
        return 0;
    }
    
    for(i = 1; i < (uint32_t) argc; i++)
    {
        if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--show-comments"))
            cfg_show_comments = true;
        else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--single-local-search"))
            cfg_single_local_search = true;
        else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--max-component"))
            cfg_score_max_comp_size = true;
        else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--num-components"))
            cfg_score_max_comp_num = true;
        else if(!strcmp(argv[i], "-w") || !strcmp(argv[i], "--weights"))
            cfg_score_max_weight = true;
        else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--print-decomposition"))
            cfg_output_decomposition = true;
    }
    
    if(!load_dimacs_graph(&builder, argv[argc - 1], cfg_show_comments))
    {
        fprintf(stderr, "Error: could not read graph from input file\n");
        return -1;
    }
    
    gbuild_create_graph(&builder, &graph);
    gbuild_destroy(&builder);
    
    /* the first subgraph contains all vertices */
    bitset_init(&vertices, GRAPH_NUM_VERTICES(&graph));
    bitset_set_all(&vertices);
    subgraph_init(&full_graph, &vertices, GRAPH_NUM_VERTICES(&graph), graph_num_edges(&graph));
    
    if(cfg_score_max_weight)
    {
        weights = (uint32_t *) malloc(sizeof(uint32_t) * graph_num_vertices(&graph));
        if(weights == NULL)
        {
            fprintf(stderr, "[error] could not allocate memory to store vertex weights\n");
            exit(0);
        }
        
        for(i = 0; i < graph_num_vertices(&graph); i++)
            weights[i] = 0;
    }
    time_start = current_timestamp();
    if(!graph_is_connected(&graph))
    {
        separator_t current_separator;

        fprintf(stdout, "[info] graph is not connected\n");
        
        bitset_init(&(current_separator.vertices), GRAPH_NUM_VERTICES(&graph));
        current_separator.greatest_component = 0;
        current_separator.num_vertices = 0;
        separator_construct_components(&graph, &full_graph, &current_separator);
        
        fprintf(stdout, "[info] decomposed graph into %u components\n", stack_height(&(current_separator.components)));

        td = 0;
        for(i = 0; i < stack_height(&(current_separator.components)); i++)
        {
            uint32_t component_td;
            subgraph_t *component = stack_get_element_ptr(&(current_separator.components), i);
            component_td = td_balanced_sep(&graph, component, &tree, weights);
            component_td = make_nice(&graph, component, &tree);
            
            fprintf(stdout, "[info] component %u: treedepth %u\n", i, component_td);

            if(component_td > td)
                td = component_td;

            if(cfg_output_decomposition)
                print_decomposition(&graph, argv[argc - 1], tree);

            rtree_free(tree);
        }
        
        separator_destroy(&current_separator);
    }
    else
    {
        td_balanced_sep(&graph, &full_graph, &tree, weights);
        td = make_nice(&graph, &full_graph, &tree);
        
        if(!tree)
        {
            fprintf(stderr, "Error: no treedepth decomposition was returned, that's weird\n");
            exit(0);
        }
        
        if(cfg_output_decomposition)
            print_decomposition(&graph, argv[argc - 1], tree);
        
        rtree_free(tree);
    }

    time_end = current_timestamp();
    
    fprintf(stderr, "[info] vertices: %u, edges: %u, treedepth: %u, total computation time: %lld.", graph_num_vertices(&graph), graph_num_edges(&graph), td, (time_end - time_start) / 1000);
    time_start = (time_end - time_start) % 1000;
    if(time_start < 10)
        fprintf(stderr, "00%llds\n", time_start);
    else if(time_start < 100)
        fprintf(stderr, "0%llds\n", time_start);
    else if(time_start < 1000)
        fprintf(stderr, "%llds\n", time_start);
    
    if(weights)
        free(weights);

    bitset_destroy(&vertices);
    subgraph_destroy(&full_graph);
    graph_destroy(&graph);

    return 0;
}

