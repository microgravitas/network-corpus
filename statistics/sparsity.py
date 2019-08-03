
def compute_degen(graph, filename, logger, timeout=None) -> min:
    """
        Computes the degeneracy of the graph
    """
    from dtf.graph import OGraph
    res = OGraph.ldo(graph)
    return max(res.in_degrees())


def compute_dtf2(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the max indegree of a 2-augmentation.  	"""
 	return dtf(filename, 2, logger, None)

def compute_dtf3(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the max indegree of a 3-augmentation.  	"""
 	return dtf(filename, 3, logger, None) 	

def compute_dtf4(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the max indegree of a 4-augmentation.  	"""
 	return dtf(filename, 4, logger, None)  	

def compute_dtf5(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the max indegree of a 5-augmentation.  	"""
 	return dtf(filename, 5, logger, None)   	

def dtf(filename, r, logger, timeout=None) -> min:
 	"""
		Computes the max indegree of an r-augmentation.
 	"""
 	from .dtf.rhizome import Rhizome
 	import sys
 	
 	with Rhizome.from_file(filename) as rhizome:
 		rhizome.augment(r);
 		return rhizome.max_in_degree() 		

def compute_domset(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the size of an (approximate) dominating set.  	"""
 	return dtf(filename, 1, logger, None)

def compute_domset2(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the size of an (approximate) 2-dominating set.  	"""
 	return dtf(filename, 2, logger, None)

def compute_domset3(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the size of an (approximate) 2-dominating set.  	"""
 	return dtf(filename, 3, logger, None) 	

def compute_domset4(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the size of an (approximate) 2-dominating set.  	"""
 	return dtf(filename, 4, logger, None) 	

def compute_domset5(graph, filename, logger, timeout=None) -> min:
 	"""		Computes the size of an (approximate) 2-dominating set.  	"""
 	return dtf(filename, 5, logger, None) 	 	

def domset(filename, r, logger, timeout=None) -> min:
 	"""
		Computes an approximate r-domset.
 	"""
 	from .dtf.rhizome import Rhizome
 	import sys
 	
 	with Rhizome.from_file(filename) as rhizome:
 		ds = rhizome.domset(r);
 		return len(ds)


def compute_frag(graph, filename, logger, timeout=None):     
    """ 
        Computer how many high-degree vertices have to be removed
        in order to have every connected component to be of size at 
        most n/2 
    """
    return frag(graph, .5)

def frag(graph, thresh) -> min: 
    graph = graph.copy()
    n = len(graph)
    degrees = sorted([(graph.degree(u), u) for u in graph], reverse=True)

    for _, u in degrees:
        graph.remove_node(u)
        max_compsize = max(len(comp) for comp in graph.get_components())
        if max_compsize/n <= thresh:
            break
    return n - len(graph)
