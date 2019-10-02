import itertools
from collections import defaultdict as defaultdict

class Graph:
    def __init__(self):
        self.adj = defaultdict(set)
        self.nodes = set()

    @staticmethod
    def from_directed(g):
        res = Graph()
        for u in g:
            res.add_node(u)
        for e in g.arcs():
            u,v = e[:2]
            res.add_edge(u,v)
        return res        

    def __contains__(self,u):
        return u in self.nodes

    def __iter__(self):
        return iter(self.nodes)

    def __len__(self):
        return len(self.nodes)
    
    def get_max_id(self):
        return max(self.nodes)

    def copy(self):
        res = Graph()
        for v in self.nodes:
            res.add_node(v)
        for u,v in self.edges():
            res.add_edge(u,v)
        return res

    def edges(self):
        for u in self:
            for v in self.adj[u]:
                if u <= v:
                    yield (u,v)

    def num_edges(self):
        return sum(1 for _ in self.edges())

    def add_node(self,u):
        self.nodes.add(u)

    def remove_node(self, u):
        for v in self.adj[u]:
            self.adj[v].discard(u)

        del self.adj[u]
        self.nodes.discard(u)

    def add_edge(self,u,v):
        self.nodes.add(u)
        self.nodes.add(v)        
        self.adj[u].add(v)
        self.adj[v].add(u)

    def remove_edge(self,u,v):
        self.adj[u].discard(v)
        self.adj[v].discard(u)

    def remove_loops(self):
        for v in self:
            self.remove_edge(v,v)

    def adjacent(self,u,v):
        return v in self.adj[u]

    def neighbours(self,u):
        return frozenset(self.adj[u])

    def neighbours_set(self, centers):
        res = set()
        for v in centers:
            res = res | self.neighbours(v)
        return (res - centers)

    def neighbours_set_closed(self, centers):
        res = set()
        for v in centers:
            res = res | self.neighbours(v)
        return res        

    def rneighbours(self,u,r):
        res = set([u])
        for _ in range(r):
            res |= self.neighbours_set_closed(res)
        return res

    def degree(self,u):
        return len(self.adj[u])

    def degree_sequence(self):
        return [ self.degree(u) for u in self.nodes] 

    def degree_dist(self):
        res = defaultdict(int)
        for u in self.nodes:
            res[self.degree(u)] += 1
        return res

    def calc_average_degree(self):
        num_edges = len( [e for e in self.edges()] )
        return float(2*num_edges) / len(self.nodes)

    def subgraph(self, vertices):
        res = Graph()
        selected = set(vertices)
        for v in self:
            if v in selected:
                res.add_node(v)

        for u,v in self.edges():
            if u in selected and v in selected:
                res.add_edge(u,v)
        return res

    # Returns graph with node indices from 0 to n-1 and 
    # a mapping dictionary to recover the original ids
    def normalize(self):
        res = Graph()
        backmapping = dict(enumerate(self))
        mapping = dict( (k,v) for (v,k) in backmapping.items() )

        for u in self:
            res.nodes.add( mapping[u] )
        for u,v in self.edges():
            res.add_edge( mapping[u], mapping[v] )

        return res, backmapping


    def get_components(self, vertices=None):
        if vertices == None:
            vertices = set(self.nodes)
        else:
            vertices = set(vertices)

        while len(vertices) > 0:
            comp = set([ vertices.pop()])
            exp = self.neighbours_set(comp) & vertices
            while len(exp) > 0:
                comp.update( exp ) 
                exp = self.neighbours_set(comp) & vertices

            vertices = vertices - comp
            yield comp

    def deep_ldo(self, ldo_depth=0):
        if ldo_depth == 0:
            return OGraph.ldo(self)

        augg = DTFGraph.from_ograph( OGraph.ldo(self) )
        augg._reserve_weight(ldo_depth+1)
        for d in range(1,ldo_depth+1):
            augg.dtf_step(d+1, 0)
        reoriented = OGraph.ldo( Graph.from_directed(augg) ) # Create LDO from augg graph
        return reoriented.edge_subgraph(self.edges()) # Translate to original graph         

class DTFGraph(object):
    def __init__(self, nodes):
        self.nodes = set(nodes)
        self.inarcs = defaultdict(dict)
        self.inarcs_weight = [None, defaultdict(set)]

    @staticmethod
    def from_graph(g):
        return DTFGraph.from_ograph( OGraph.ldo(g) )

    @staticmethod
    def from_ograph(g):
        res = DTFGraph(g.nodes)
        for u,v in g.arcs():
            res._add_arc(u,v,1)
        return res

    def __contains__(self,u):
        return u in self.nodes

    def __len__(self):
        return len(self.nodes)

    def __iter__(self):
        return iter(self.nodes)

    def add_node(self,u):
        self.nodes.add(u)
        return self

    def add_arc(self, u, v, weight):
        assert u != v
        self._reserve_weight(weight)
        self._add_arc(u,v,weight)

    def _reserve_weight(self, weight):
        while len(self.inarcs_weight) <= weight:
            self.inarcs_weight.append(defaultdict(set))

    def _add_arc(self, u, v, weight):
        self.inarcs[v][u] = weight
        self.inarcs_weight[weight][v].add(u)

    def remove_arc(self,u,v):
        if u not in self.inarcs[v]:
            return
        weight = self.inarcs[v][u]
        del self.inarcs[v][u]
        self.inarcs_weight[weight][v].remove(u)
        assert not self.adjacent(u,v)
        return self

    def arcs_weight(self, weight):
        for u in self:
                for v, w in self.in_neighbours(u):
                    if w == weight:
                        yield (v, u)

    def arcs(self):
        for u in self:
            for v, w in self.in_neighbours(u):
                yield (v, u, w)
            

    def num_arcs(self):
        return sum( 1 for _ in self.arcs() )

    # Return the arc weight of uv or None if it does not exit
    def weight(self,u,v):
        return self.inarcs[v][u] if u in self.inarcs[v] else None

    # Returns whether the arc uv exists
    def adjacent(self,u,v):
        return u in self.inarcs[v]

    def in_neighbours(self,u):
        inbs = self.inarcs[u]
        for v in inbs:
            yield v, inbs[v]

    def in_neighbours_weight(self,u,weight):
        inbs = self.inarcs_weight[weight][u]
        for v in inbs:
            yield v

    def in_degree(self,u):
        return len(self.inarcs[u])

    def in_degree_sequence(self):
        return [ self.in_degree(u) for u in self.nodes]

    def in_degree_dist(self):
        res = defaultdict(int)
        for u in self.nodes:
            res[self.in_degree(u)] += 1
        return res

    def degree_dist(self):
        degrees = defaultdict(int)
        for u in self.nodes:
            for w in self.inarcs[u]:
                degrees[u] += 1
                degrees[w] += 1
        res = defaultdict(int)
        for u in self.nodes:
            res[degrees[u]] += 1
        return res

    # Returns either the distance between u,v in the original
    # graph (if dist <= # of augmentation steps) or 'inf' otherwise.
    def distance(self,u,v):
        distance = float('inf')
        Nu = {}
        for x,d in self.in_neighbours(u):
            Nu[x] = d
            if x == v:
                distance = d
        for x,d in self.in_neighbours(v):
            if x == u:
                distance = min(distance,d)
            elif x in Nu:
                distance = min(distance,d+Nu[x])
        return distance

    def common_in_neighbours(self,u,v):
        Nu = {}
        for x,d in self.in_neighbours(u):
            Nu[x] = d
        if v in Nu:
            del Nu[v]

        for x,dd in self.in_neighbours(v):
            if x in Nu:
                yield x, Nu[x], dd

    def copy(self):
        res = DTFGraph(self.nodes)
        for u,v,d in self.arcs():
            res.add_arc(u,v,d)
        return res

    # Returns an undirected copy
    def undirected(self):
        res = Graph()
        for v in self.nodes:
            res.nodes.add(v) # For degree-0 nodes!
        for u,v,_ in self.arcs():
            res.add_edge(u,v)
        return res

    # Returns vertices y \in N^{--}(u) \setminus N^-(u) such that
    # the weights of arcs (y,x), (x,u) add up to 'weight'
    def _trans_tails(self,u,weight):
        # We first collect all candidates vertices and then remove 'inbs'
        inbs = frozenset(self.inarcs[u].keys()) | frozenset([u])
        cands = set()
        for wy in range(1, weight):
            wx = weight-wy
            for y in self.inarcs_weight[wy][u]:
                cands.update(self.inarcs_weight[wx][y])
        return cands - inbs

    # Returns fraternal pairs (x,y) in N^-(u) whose weightsum
    # is precisely 'weight' > 2 and in which one arc has weight 'weight-1'
    # and the other '1' (this is sufficient to maintain the distance-property
    # of dtf-augmentations)
    def _frat_pairs(self, u, weight):
        # Since weight != 2, the numbers 1 and weight-1 are different and
        # therefore we draw the potential fraternal pairs from differently weighted in-neighbourhoods
        for wx in [1,weight-1]:
            wy = weight-wx
            for x in self.inarcs_weight[wx][u]:
                for y in self.inarcs_weight[wy][u]:
                    # if not (self.adjacent(x,y) or self.adjacent(y,x)):
                    if y not in self.inarcs[x] and x not in self.inarcs[y] : # Inlined by hand since performance-critical                        
                        yield (x, y)

    # The same as _frat_pairs but for 'weight' == 2
    def _frat_pairs2(self, u):
        # Since 2-1 = 1, we draw fraternal candidates from the weight-1
        # in-neighbourhood
        inbs = frozenset(self.inarcs_weight[1][u])
        for x, y in itertools.combinations(inbs, 2):
            # if not (self.adjacent(x,y) or self.adjacent(y,x)):
            if y not in self.inarcs[x] and x not in self.inarcs[y] : # Inlined by hand since performance-critical
                yield (x, y)

    def dtf_step(self, dist, ldo_depth=2):
        fratGraph = Graph()
        newTrans = {}

        self._reserve_weight(dist+1)
        for v in self.nodes:
            for x in self._trans_tails(v, dist):
                newTrans[(x,v)] = dist 
            if dist == 2:
                for x,y in self._frat_pairs2(v):
                    fratGraph.add_edge(x,y)
            else:
                for x,y in self._frat_pairs(v, dist):
                    fratGraph.add_edge(x,y)

        for (s, t) in newTrans:
            self._add_arc(s, t, dist)
            fratGraph.remove_edge(s,t)
        
        fratDigraph = fratGraph.deep_ldo(ldo_depth)
        
        for s,t in fratDigraph.arcs():
            self._add_arc(s,t,dist)        


class OGraph:
    def __init__(self):
        self.arcset = set()
        self.in_arcs = defaultdict(set)
        self.out_arcs = defaultdict(set)
        self.nodes = set()

    @staticmethod
    def on(verts):
        res = OGraph()
        for v in verts:
            res.add_node(v)
        return res

    @staticmethod
    def ldo(graph):
        # We sort vertices into degree-buckets with exponentially
        # expanding range in order to decrease memory consumption and
        # long searches for the next non-empty bucket. Indiced below 'small'
        # are simply mapped to themselves.
        small = 2**5
        def calc_index(i): 
            return min(i,small) + max(0, i-small).bit_length()

        res = OGraph.on(graph)
        degdict = {}
        buckets = defaultdict(set)
        for v in graph:
            d = degdict[v] = graph.degree(v)
            buckets[calc_index(d)].add(v)
        
        seen = set()
        for i in range(0, len(graph)):
            d = 0
            while len(buckets[d]) == 0: 
                d += 1
            v = buckets[d].pop()
            
            for u in graph.neighbours(v):
                if u in seen:
                    continue
                d = degdict[u]
                old_index, new_index = calc_index(d), calc_index(d-1)
                if old_index != new_index:
                    buckets[old_index].remove(u)
                    buckets[new_index].add(u)
                degdict[u] -= 1
                # Orient edges towards v
                res.add_arc(u,v)

            seen.add(v)
        return res 

    def __contains__(self,u):
        return u in self.nodes

    def __iter__(self):
        return iter(self.nodes)

    def __len__(self):
        return len(self.nodes)

    def arcs(self):
        return iter(self.arcset)

    def add_node(self,u):
        self.nodes.add(u)
        return self

    def get_arc(self, u, v):
        if (u,v) in self.arcset:
            return (u,v)
        elif (v,u) in self.arcset:
            return (v,u)
        raise Exception("Edge {},{} not part of ograph".format(u,v))

    def in_degree(self, u):
        return len(self.in_arcs[u])

    def in_degrees(self):
        return [len(self.in_arcs[u]) for u in self]

    def out_degree(self, u):
        return len(self.out_arcs[u])  

    def in_neighbours(self, u):
        for v,_ in self.in_arcs[u]:
            yield v

    def out_neighbours(self, u):
        for _,v in self.out_arcs[u]:
            yield v            

    def remove_arc(self, u, v):
        arc = (u,v)
        assert(arc in self.arcset)
        self.arcset.remove(arc)
        self.out_arcs[u].remove(arc)
        self.in_arcs[v].remove(arc)
        return self

    def add_arc(self, u, v):
        self.add_node(u).add_node(v)
        arc = (u,v)
        assert(arc not in self.arcset)
        self.arcset.add(arc)
        self.out_arcs[u].add(arc)
        self.in_arcs[v].add(arc)
        return self

    def edge_subgraph(self, edges):
        # Returns the subgraph formed by the given edges,
        # but retaining the orientation of this ograph.
        res = OGraph.on(self.nodes)
        for u,v in edges:
            res.add_arc(*self.get_arc(u,v))
        return res 