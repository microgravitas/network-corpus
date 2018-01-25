#include <assert.h>
#include "graph.h"

int graph_cmp_vertex_labels(const void *a, const void *b)
{
    const vertex_label_t *f = (const vertex_label_t *) a;
    const vertex_label_t *g = (const vertex_label_t *) b;
    
    if(*f < *g)
        return -1;
    else if(*f > *g)
        return 1;
    else
        return 0;
}

int graph_cmp_vertices(const void *a, const void *b)
{
    const vertex_t *f = (const vertex_t *) a;
    const vertex_t *g = (const vertex_t *) b;
    
    if(*f < *g)
        return -1;
    else if(*f > *g)
        return 1;
    else
        return 0;
}

int graph_cmp_edges(const void *a, const void *b)
{
    const edge_t *f = (const edge_t *) a;
    const edge_t *g = (const edge_t *) b;
    
    if(f->from < g->from)
        return -1;
    else if(f->from > g->from)
        return 1;
    else if(f->to < g->to)
        return -1;
    else if(f->to > g->to)
        return 1;
    else
        return 0;
}

void gbuild_init(gbuild_t *builder)
{
    builder->max_vertex_label = 0;
    stack_init(&(builder->vertex_labels), sizeof(vertex_label_t));
    stack_init(&(builder->edges), sizeof(edge_t));
}

void gbuild_destroy(gbuild_t *builder)
{
    stack_destroy(&(builder->vertex_labels));
    stack_destroy(&(builder->edges));
}

void gbuild_add_vertex(gbuild_t *builder, vertex_label_t vertex_label)
{
    if(vertex_label > builder->max_vertex_label)
        builder->max_vertex_label = vertex_label;
    
    /* check if vertex was added before, only add if not already contained in list */
    if(!stack_binsearch(&(builder->vertex_labels), graph_cmp_vertex_labels, &vertex_label, NULL))
    {
        /* vertex_label is not in list yet, so add it and resort the list */
        stack_push(&(builder->vertex_labels), &vertex_label);
        stack_sort(&(builder->vertex_labels), graph_cmp_vertex_labels);
    }
}

void gbuild_add_edge(gbuild_t *builder, vertex_label_t from, vertex_label_t to)
{
    bool add_from, add_to;
    edge_t edge;
    
    if(from > to)
    {
        vertex_label_t tmp = from;
        from = to;
        to = tmp;
    }
    
    add_from = !stack_binsearch(&(builder->vertex_labels), graph_cmp_vertex_labels, &from, NULL);
    add_to = !stack_binsearch(&(builder->vertex_labels), graph_cmp_vertex_labels, &to, NULL);
    
    if(add_from)
        stack_push(&(builder->vertex_labels), &from);
    
    if(add_to)
        stack_push(&(builder->vertex_labels), &to);
    
    if(add_from || add_to)
        stack_sort(&(builder->vertex_labels), graph_cmp_vertex_labels);

    edge.from = from;
    edge.to = to;
        
    stack_push(&(builder->edges), &edge);

}

uint32_t gbuild_num_vertices(gbuild_t *builder)
{
    return stack_height(&(builder->vertex_labels));
}

uint32_t gbuild_num_edges(gbuild_t *builder)
{
    return stack_height(&(builder->edges));
}

bool gbuild_is_trivial(gbuild_t *builder)
{
    return (stack_height(&(builder->edges)) == 0);
}

void gbuild_create_graph(gbuild_t *builder, graph_t *graph)
{
    uint32_t num_vertices = 0, num_edges = 0, num_builder_edges = 0, pos;
    uint32_t *vertex_degrees, i;
    edge_t *edge_iter;
    
    graph->n = 0;
    graph->m = 0;
    graph->labels = NULL;
    graph->positions = NULL;
    graph->edges = NULL;
    
    num_vertices = gbuild_num_vertices(builder);
    if(num_vertices == 0)
        return;
    else
        graph->n = num_vertices;
        
    graph->labels = (vertex_label_t *) malloc(sizeof(vertex_label_t) * num_vertices);
    if(graph->labels == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store vertex labels\n");
        exit(0);
    }
    
    memcpy(graph->labels, builder->vertex_labels.data, sizeof(vertex_label_t) * num_vertices);
    
    /* if graph is trivial, we have isolated vertices and then store no edges at all */
    if(gbuild_is_trivial(builder))
    {
        graph->m = 0;
        graph->positions = (uint32_t *) malloc(sizeof(uint32_t) * num_vertices);
        if(graph->positions == NULL)
        {
            fprintf(stderr, "Error: could not allocate memory to store edge positions\n");
            exit(0);
        }
        
        /* no edges => all positions = 0 */
        for(i = 0; i < num_vertices; i++)
            graph->positions[i] = 0;
            
        return;
    }
    
    /* count vertex degrees, use auxiliary array, size is O(n) */
    vertex_degrees = (uint32_t *) malloc(sizeof(uint32_t) * num_vertices);
    if(vertex_degrees == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store vertex degrees\n");
        exit(0);
    }
    
    /* set all degrees to zero */
    for(i = 0; i < num_vertices; i++)
        vertex_degrees[i] = 0;
    
    /* sort edge list, so we can iterate and skip duplicates */
    stack_sort(&(builder->edges), graph_cmp_edges);
    num_builder_edges = gbuild_num_edges(builder);
    
    edge_iter = (edge_t *) builder->edges.data;
    num_edges = 0;
    
    for(i = 0; i < num_builder_edges; i++, edge_iter++)
    {
        vertex_t vert_from = 0, vert_to = 0;
        
        /* skip duplicates */
        if((i > 0) && (graph_cmp_edges(edge_iter - 1, edge_iter) == 0))
            continue;
        
        /* we assume all vertex_labels can be found in the list, NO ERROR CHECKING */
        graph_get_vertex_by_label(graph, edge_iter->from, &vert_from);
        graph_get_vertex_by_label(graph, edge_iter->to, &vert_to);
        
        /* increase degree count of edge endpoints */
        vertex_degrees[vert_from]++;
        vertex_degrees[vert_to]++;
        num_edges++;
    }
    
    graph->m = num_edges;
    
    /* allocate memory */
    graph->positions = (uint32_t *) malloc(sizeof(uint32_t) * num_vertices);
    if(graph->positions == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store edge positions\n");
        exit(0);
    }
    
    graph->positions[0] = 0;
    pos = vertex_degrees[0];
    for(i = 1; i < num_vertices; i++)
    {
        graph->positions[i] = pos;
        pos = pos + vertex_degrees[i];
    }
    
    graph->edges = (vertex_t *) malloc(sizeof(vertex_t) * 2 * num_edges);
    if(graph->edges == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store edges\n");
        exit(0);
    }
    
    /* go through edge list, insert edges into edge list */
    edge_iter = (edge_t *) builder->edges.data;
    
    for(i = 0; i < num_builder_edges; i++, edge_iter++)
    {
        vertex_t vert_from = 0, vert_to = 0;
        
        /* skip duplicates */
        if((i > 0) && (graph_cmp_edges(edge_iter - 1, edge_iter) == 0))
            continue;
        
        /* we assume all vertex_labels can be found in the list, NO ERROR CHECKING */
        graph_get_vertex_by_label(graph, edge_iter->from, &vert_from);
        graph_get_vertex_by_label(graph, edge_iter->to, &vert_to);
        
        graph->edges[graph->positions[vert_from]] = vert_to;
        graph->edges[graph->positions[vert_to]] = vert_from;
        graph->positions[vert_from]++;
        graph->positions[vert_to]++;
    }
    
    for(i = 0; i < num_vertices; i++)
        graph->positions[i] = graph->positions[i] - vertex_degrees[i];
    
    free(vertex_degrees);
}

void graph_destroy(graph_t *graph)
{
    if(graph->labels)
    {
        free(graph->labels);
        graph->labels = NULL;
    }
    
    if(graph->positions)
    {
        free(graph->positions);
        graph->positions = NULL;
    }
    
    if(graph->edges)
    {
        free(graph->edges);
        graph->edges = NULL;
    }
    
    graph->n = 0;
    graph->m = 0;
}

void graph_print(const graph_t const *graph)
{
    vertex_t vert_iter;
    uint32_t iter_start, iter_end;
    
    for(vert_iter = 0; vert_iter < GRAPH_NUM_VERTICES(graph); vert_iter++)
    {
        graph_edge_iter(graph, vert_iter, &iter_start, &iter_end);
        for(; iter_start < iter_end; iter_start++)
        {
            //if(vert_iter > graph->edges[iter_start])
             //   continue;

            fprintf(stdout, "[%u, %u]\n", graph->labels[vert_iter], graph->labels[graph->edges[iter_start]]);
        }
    }
}

uint32_t graph_num_vertices(const graph_t const *graph)
{
    return graph->n;
}

uint32_t graph_num_edges(const graph_t const *graph)
{
    return graph->m;
}

uint32_t graph_degree(const graph_t const *graph, const vertex_t vertex)
{
    if(graph->n <= vertex)
        return 0;
    
    if((vertex + 1) == graph->n)
        return (2 * graph->m - graph->positions[vertex]);
    else
        return (graph->positions[vertex + 1] - graph->positions[vertex]);
}

bool graph_edge_iter(const graph_t const *graph, const vertex_t vertex, uint32_t *iter_start, uint32_t *iter_end)
{
    if(iter_start)
        *iter_start = 0;
    if(iter_end)
        *iter_end = 0;
    
    if(graph->n <= vertex)
        return false;
    
    *iter_start = graph->positions[vertex];
    *iter_end = *iter_start + graph_degree(graph, vertex);
    return true;
}

bool graph_get_vertex_by_label(const graph_t const *graph, const vertex_label_t vertex_label, vertex_t *result)
{
    uint32_t l, m, r;
    int res;
    
    l = 0;
    r = graph->n;
    
    while(l < r)
    {
        m = (l + r) / 2;
        res = graph_cmp_vertex_labels(&(graph->labels[m]), &vertex_label);
        
        if(res < 0)
            l = m + 1;
        else if(res > 0)
            r = m;
        else
        {
            if(result)
                *result = m;
            return true;
        }
    }
    
    return false;
}

vertex_t graph_get_edge(const graph_t const *graph, const uint32_t edge)
{
    if(edge >= 2 * graph->m)
    {
        fprintf(stderr, "Error: edge_id is out of bounds\n");
        exit(0);
    }
    return graph->edges[edge];
}

bool graph_is_connected(const graph_t const *graph)
{
    uint32_t num_vertices;
    bitset_t visited;
    stack_t dfs_stack;
    dfs_state_t state;
    vertex_t neighbor;
    
    num_vertices = GRAPH_NUM_VERTICES(graph);
    
    bitset_init(&visited, num_vertices);
    stack_init(&dfs_stack, sizeof(dfs_state_t));
    
    graph_edge_iter(graph, 0, &state.edge_start, &state.edge_end);
    bitset_set(&visited, 0);
    stack_push(&dfs_stack, &state);
    num_vertices = 1;
    
    while(stack_pop(&dfs_stack, &state))
    {
        while(state.edge_start < state.edge_end)
        {
            neighbor = graph_get_edge(graph, state.edge_start);
            state.edge_start++;

            if(bitset_get(&visited, neighbor))
                continue;

            stack_push(&dfs_stack, &state);
            bitset_set(&visited, neighbor);
            graph_edge_iter(graph, neighbor, &state.edge_start, &state.edge_end);
            num_vertices++;
        }
    }
    
    stack_destroy(&dfs_stack);
    bitset_destroy(&visited);
    
    return (num_vertices == GRAPH_NUM_VERTICES(graph));
}

void subgraph_init(subgraph_t *subgraph, const bitset_t const *vertices, const uint32_t num_vertices, const uint32_t num_edges)
{
    bitset_init_copy(&(subgraph->vertices), vertices);
    subgraph->num_vertices = num_vertices;
    subgraph->num_edges = num_edges;
}

void subgraph_destroy(subgraph_t *subgraph)
{
    bitset_destroy(&(subgraph->vertices));
}

bool subgraph_is_tree(const subgraph_t const *subgraph)
{
    return ((subgraph->num_edges + 1) == subgraph->num_vertices);
}

bool subgraph_is_clique(const subgraph_t const *subgraph)
{
    return (subgraph->num_edges == ((subgraph->num_vertices * (subgraph->num_vertices - 1) / 2)));
}

void subgraph_dfs(const graph_t const *graph, const subgraph_t const *subgraph)
{
    bitset_index_t root;
    bitset_t visited;
    stack_t dfs_stack;
    dfs_state_t state;
    vertex_t neighbor;
    
    /* find some vertex that we can use as root */
    if(!bitset_find_set_bit(&(subgraph->vertices), &root))
    {
        fprintf(stderr, "Error: can't find vertex in subgraph to use as root\n");
        exit(0);
    }
    
    bitset_init(&visited, GRAPH_NUM_VERTICES(graph));
    stack_init(&dfs_stack, sizeof(dfs_state_t));
    
    state.vertex = (vertex_t) root;
    graph_edge_iter(graph, state.vertex, &state.edge_start, &state.edge_end);
    stack_push(&dfs_stack, &state);
    
    /* mark our root as visited */
    bitset_set(&visited, state.vertex);
    
    while(stack_pop(&dfs_stack, &state))
    {
        while(state.edge_start < state.edge_end)
        {
            neighbor = graph_get_edge(graph, state.edge_start);
            state.edge_start++;

            if(bitset_get(&visited, neighbor))
                continue;
            if(!bitset_get(&(subgraph->vertices), neighbor))
                continue;

            stack_push(&dfs_stack, &state);
            
            bitset_set(&visited, neighbor);
            state.vertex = neighbor;
            graph_edge_iter(graph, neighbor, &state.edge_start, &state.edge_end);
        }
    }
    
    stack_destroy(&dfs_stack);
    bitset_destroy(&visited);
}

void separator_init_copy(separator_t *separator, const separator_t const *source)
{
    uint32_t i;
    subgraph_t *component, component_copy;
    
    /* copy separator vertices */
    bitset_init_copy(&(separator->vertices), &(source->vertices));
    separator->num_vertices = source->num_vertices;
    separator->greatest_component = source->greatest_component;
    
    /* copy components */
    stack_init(&(separator->components), sizeof(subgraph_t));
    for(i = 0; i < stack_height(&(source->components)); i++)
    {
        component = stack_get_element_ptr(&(source->components), i);
        subgraph_init(&component_copy, &(component->vertices), component->num_vertices, component->num_edges);
        stack_push(&(separator->components), &component_copy);
    }
}

void separator_destroy(separator_t *separator)
{
    subgraph_t subgraph;
    
    bitset_destroy(&(separator->vertices));
    
    while(stack_pop(&(separator->components), &subgraph))
        subgraph_destroy(&subgraph);

    stack_destroy(&(separator->components));
    separator->greatest_component = 0;
}

void separator_destroy_simple(separator_t *separator)
{
    bitset_destroy(&(separator->vertices));
    separator->greatest_component = 0;
}

void separator_destroy_components(separator_t *separator)
{
    subgraph_t subgraph;

    while(stack_pop(&(separator->components), &subgraph))
        subgraph_destroy(&subgraph);

    stack_destroy(&(separator->components));
    separator->greatest_component = 0;
}

void separator_construct_components(const graph_t const *graph, const subgraph_t const *subgraph, separator_t *separator)
{
    bitset_t induced_subgraph, current_subgraph, vertex_iterator;
    uint32_t num_vertices, num_edges;
    stack_t dfs_stack;
    dfs_state_t state;
    bitset_index_t last_vertex, next_vertex;
    subgraph_t new_subgraph;
    vertex_t neighbor;
    
    /* create stacks for components and dfs trees */
    stack_init(&(separator->components), sizeof(subgraph_t));
    
    /* copy subgraph and remove separator */
    bitset_init_copy(&induced_subgraph, &(subgraph->vertices));
    bitset_remove_set(&induced_subgraph, &(separator->vertices));
    
    /* initialize dfs stack */
    stack_init(&dfs_stack, sizeof(dfs_state_t));
    
    bitset_init(&current_subgraph, GRAPH_NUM_VERTICES(graph));
    
    /* create vertex iterator */
    bitset_init_copy(&vertex_iterator, &induced_subgraph);
    last_vertex = 0;
    
    while(bitset_iterate_set_and_clear(&vertex_iterator, &next_vertex, &last_vertex))
    {
        /* reset current_subgraph, it is going to contain all vertices from this component */
        bitset_clear_all(&current_subgraph);
        num_vertices = 1;
        num_edges = 0;
        
        /* create dfs tree for the root and push it onto the dfs stack */
        state.vertex = (vertex_t) next_vertex;
        graph_edge_iter(graph, state.vertex, &state.edge_start, &state.edge_end);
        stack_push(&dfs_stack, &state);
        
        /* mark our root as visited */
        bitset_set(&current_subgraph, state.vertex);
        
        while(stack_pop(&dfs_stack, &state))
        {
            while(state.edge_start < state.edge_end)
            {
                neighbor = graph_get_edge(graph, state.edge_start);
                state.edge_start++;

                if(bitset_get(&current_subgraph, neighbor))
                {
                    num_edges++;
                    continue;
                }

                /* skip vertices that are not part of our subgraph */                
                if(!bitset_get(&induced_subgraph, neighbor))
                    continue;

                num_vertices++;
                num_edges++;
                
                /* push old node back onto the stack */
                stack_push(&dfs_stack, &state);
                
                /* mark neighbor as visited */
                bitset_set(&current_subgraph, neighbor);
                bitset_clear(&vertex_iterator, neighbor);
                
                state.vertex = neighbor;
                graph_edge_iter(graph, neighbor, &state.edge_start, &state.edge_end);
            }
        }
        
        subgraph_init(&new_subgraph, &current_subgraph, num_vertices, num_edges / 2);
        stack_push(&(separator->components), &new_subgraph);
    }
    bitset_destroy(&vertex_iterator);
    
    stack_destroy(&dfs_stack);
    bitset_destroy(&current_subgraph);
    bitset_destroy(&induced_subgraph);
}

/* performs DFS in component that contains root and adds all vertices to separator, that are contained in 'subgraph' but not in 'separated_subgraph' */
void separator_dfs(const graph_t const *graph, const subgraph_t const *subgraph, const bitset_t const *separated_subgraph, bitset_t *visited, const bitset_index_t root, separator_t *separator)
{
    stack_t dfs_stack;
    dfs_state_t state;
    vertex_t neighbor;
    
    stack_init(&dfs_stack, sizeof(dfs_state_t));
    bitset_init(&(separator->vertices), GRAPH_NUM_VERTICES(graph));
    
    separator->num_vertices = 0;
    
    state.vertex = (vertex_t) root;
    graph_edge_iter(graph, state.vertex, &state.edge_start, &state.edge_end);
    stack_push(&dfs_stack, &state);
    
    bitset_set(visited, root);
    
    while(stack_pop(&dfs_stack, &state))
    {
        while(state.edge_start < state.edge_end)
        {
            neighbor = graph_get_edge(graph, state.edge_start);
            state.edge_start++;
            
            /* skip all vertices that are not part of this subgraph */
            if(!bitset_get(&(subgraph->vertices), neighbor))
                continue;

            /* do not visit vertices again */
            if(bitset_get(visited, neighbor))
                continue;
            
            if(!bitset_get(separated_subgraph, neighbor))
            {
                if(bitset_get(&(separator->vertices), neighbor) == false)
                {
                    bitset_set(&(separator->vertices), neighbor);
                    separator->num_vertices = separator->num_vertices + 1;
                }

                continue;
            }
            
            bitset_set(visited, neighbor);
            
            stack_push(&dfs_stack, &state);
            state.vertex = neighbor;
            graph_edge_iter(graph, state.vertex, &state.edge_start, &state.edge_end);
        }
    }
    
    stack_destroy(&dfs_stack);
    
}

void separator_construct_from_components(const graph_t const *graph, const subgraph_t const *subgraph, const bitset_t const *separated_subgraph, stack_t *separators)
{
    separator_t current_separator;
    bitset_t component_iter;
    bitset_index_t last_vertex, next_vertex;
    bitset_t visited;
    
    stack_init(separators, sizeof(separator_t));
    
    bitset_init(&visited, GRAPH_NUM_VERTICES(graph));
    bitset_init_copy(&component_iter, separated_subgraph);
    last_vertex = 0;
    
    while(bitset_iterate_set_and_clear(&component_iter, &next_vertex, &last_vertex))
    {
        if(bitset_get(&visited, (bitset_index_t) next_vertex) == true)
            continue;

        separator_dfs(graph, subgraph, separated_subgraph, &visited, next_vertex, &current_separator);
        if(current_separator.num_vertices == 0)
            fprintf(stderr, "[error] close separator has zero size\n");
/*
        else
            fprintf(stdout, "[info] separator has %u vertices\n", current_separator.num_vertices);*/

        stack_push(separators, &current_separator);
    }
    
    bitset_destroy(&visited);
    bitset_destroy(&component_iter);
}

rtree_t *rtree_alloc(void)
{
    rtree_t *tree = (rtree_t *) malloc(sizeof(rtree_t));
    if(tree == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory to store rooted tree\n");
        exit(0);
    }
    return tree;
}

rtree_t *rtree_new(vertex_t root)
{
    rtree_t *tree = rtree_alloc();
    tree->root = root;
    tree->num_children = 0;
    tree->children = NULL;
    tree->parent = NULL;
    return tree;
}

void rtree_free(rtree_t *tree)
{
    while(tree->num_children > 0)
        rtree_free(tree->children[0]);
    
    if(tree->parent)
        rtree_remove_child(tree->parent, tree);
    
    free(tree);
}

void rtree_destroy(rtree_t *tree)
{
    while(tree->num_children > 0)
        rtree_destroy(tree->children[0]);
    
    if(tree->parent)
        rtree_remove_child(tree->parent, tree);
}

void rtree_add_child(rtree_t *parent, rtree_t *child)
{
    if(parent->children)
        parent->children = (rtree_t **) realloc(parent->children, sizeof(rtree_t *) * (parent->num_children + 1));
    else
        parent->children = (rtree_t **) malloc(sizeof(rtree_t *) * (parent->num_children + 1));
    
    if(parent->children == NULL)
    {
        fprintf(stderr, "Error: could not allocate memory for rooted tree to add a new child\n");
        exit(0); // TODO: Do not return success after an error.
    }
    
    parent->children[parent->num_children] = child;
    parent->num_children++;
    
    if(child->parent)
        rtree_remove_child(child->parent, child);
    
    child->parent = parent;
}

void rtree_remove_child(rtree_t *parent, rtree_t *child)
{
    uint32_t i;
    
    assert(parent);
    assert(child);
    
    for(i = 0; i < parent->num_children; i++)
    {
        if(parent->children[i] == child)
        {
            /* put last child into place i */
            parent->children[i] = parent->children[parent->num_children - 1];
            parent->num_children--;
            if(parent->num_children > 0)
            {
                parent->children = (rtree_t **) realloc(parent->children, sizeof(rtree_t *) * parent->num_children);
                if(parent->children == NULL)
                {
                    fprintf(stderr, "Error: could not release memory after child removal (rtree)\n");
                    exit(0);
                }
            }
            else
            {
                free(parent->children);
                parent->children = NULL;
            }
            
            child->parent = NULL;
            return;
        }
    }
}

void rtree_print(rtree_t *tree)
{
    uint32_t i;
    fprintf(stdout, "node: %u, children: %u, [", tree->root, tree->num_children);
    for(i = 0; i < tree->num_children; i++)
    {
        if(i > 0)
            fprintf(stdout, ", %u", tree->children[i]->root);
        else
            fprintf(stdout, "%u", tree->children[i]->root);
    }
    fprintf(stdout, "]\n");
    
    for(i = 0; i < tree->num_children; i++)
        rtree_print(tree->children[i]);
}

uint32_t rtree_height(rtree_t *tree)
{
    uint32_t height = 0, h, i;
    
    for(i = 0; i < tree->num_children; i++)
    {
        h = rtree_height(tree->children[i]);
        if(h > height)
            height = h;
    }
    
    return 1 + height;
}

