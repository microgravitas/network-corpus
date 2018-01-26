
def compute_n(graph, logger, timeout=None):
    """
        Counts the number of vertices.
    """
    return len(graph)

def compute_m(graph, logger, timeout=None):
    """
        Counts the number of edges. 
    """
    return graph.num_edges()

def compute_maxdeg(graph, logger, timeout=None):
    """
        Returns the maximum degree.
    """
    return max(graph.degree_sequence())

def compute_avgdeg(graph, logger, timeout=None):
    """
        Returns the average degree.
    """
    return graph.calc_average_degree()