
def compute_degen(graph, logger, timeout=None) -> min:
    """
        Computes the degeneracy of the graph
    """
    from dtf.graph import OGraph
    res = OGraph.ldo(graph)
    return max(res.in_degrees())