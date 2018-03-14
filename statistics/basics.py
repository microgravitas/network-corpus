
def compute_n(graph, filename, logger, timeout=None):
    """
        Counts the number of vertices.
    """
    return len(graph)

def compute_m(graph, filename, logger, timeout=None):
    """
        Counts the number of edges. 
    """
    return graph.num_edges()

def compute_maxdeg(graph, filename, logger, timeout=None):
    """
        Computes the maximum degree.
    """
    return max(graph.degree_sequence())

def compute_avgdeg(graph, filename, logger, timeout=None):
    """
        Computes the average degree.
    """
    return graph.calc_average_degree()


def compute_radius(graph, filename, logger, timeout=None):
    """
        Computes the radius (uses networkx)
    """
    import networkx as nx
    G = _to_networkx(graph)
    Gcc = sorted(nx.connected_component_subgraphs(G), key = len, reverse=True)[0]
    return nx.radius(Gcc)

def compute_assort(graph, filename, logger, timeout=None):
    """
        Computes the degree assortativity coefficient (uses networkx)
    """
    import networkx as nx
    G = _to_networkx(graph)
    return nx.degree_assortativity_coefficient(G)

def _to_networkx(graph):
    import networkx as nx
    G = nx.Graph()
    for u,v in graph.edges():
        G.add_edge(u,v)
    return G