
def compute_n(graph, **kwargs):
    """
        Counts the number of vertices.
    """
    return len(graph)

def compute_m(graph, **kwargs):
    """
        Counts the number of edges. 
    """
    return graph.num_edges()

def compute_maxdeg(graph, **kwargs):
    """
        Returns the maximum degree.
    """
    return max(graph.degree_sequence())

def compute_avgdeg(graph, **kwargs):
    """
        Returns the average degree.
    """
    return graph.calc_average_degree()