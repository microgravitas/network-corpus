
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

def dtf(filename, r, logger, timeout=None) -> min:
 	"""
		Computes the max indegree of an r-augmentation.
 	"""
 	from .dtf.rhizome import Rhizome
 	import sys
 	
 	with Rhizome.from_file(filename) as rhizome:
 		rhizome.augment(r);
 		return rhizome.max_in_degree() 		