
def compute_tw_flowcutter(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using the FlowCutter heuristic
        (https://github.com/ben-strasser/flow-cutter-pace16).
    """
    width, _, _ = tw_pace(graph, logger, "flow_cutter_pace16", timeout)
    return width

def compute_tw_foxepstein(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using Fox-Epstein heuristic
        (https://github.com/elitheeli/2016-pace-challenge)
    """
    width, _, _ = tw_pace(graph, logger, "fox_epstein_pace16", timeout)
    return width


def compute_tw_htd(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using the htd heuristic
        (https://github.com/mabseher/htd).
    """
    width, _, _ = tw_pace(graph, logger, "htd_pace16", timeout)
    return width

def tw_pace(graph, logger, program, timeout=None) -> min:
    """ 
        Wrapper for all pace16 (pace17?) treewidth heuristics
    """
    import os, sys, subprocess
    import time
    from dtf.graph import Graph

    # See https://github.com/PACE-challenge/Treewidth#input-format for
    # a description of the .gr format
    with open('tmp.gr', 'w') as filebuf:
        maxid = max(graph)
        filebuf.write("p tw {} {}\n".format(maxid+1, graph.num_edges()))
        for u,v in graph.edges():
            filebuf.write("{} {}\n".format(u+1,v+1)) # The .gr format is 1-based.        
        # filebuf.write("\n")

    if not timeout:
        timeout = 300 # The heuristic needs a timeout

    # Run heuristic for [timeout] seconds, then terminate.
    # The heuristic will return the best found solution so far via stdout.
    try:
        out = subprocess.check_output(["statistics/tw/{}".format(program)], stdin=open("tmp.gr", 'rb'), timeout=timeout)
    except subprocess.TimeoutExpired as e:
        out = e.stdout

    width = None
    bags = {}
    tree = Graph()
    for l in out.decode().split('\n'):
        if len(l) == 0 or l[0] == "c":
            continue

        tokens = l.split()
        if tokens[0] == "s":
            # Solution like has format 's td [num-bags] [max bag size] [vertices in graph]'
            assert tokens[1] == 'td'
            width = int(tokens[3])
        elif tokens[0] == "b":
            bag = int(tokens[1])
            bags[bag] = frozenset(map(int, tokens[2:]))
        else:
            assert len(tokens) == 2
            tree.add_edge(int(tokens[0]), int(tokens[1]))

    os.remove('tmp.gr')

    return width, tree, bags

def compute_td_htd(graph, logger, timeout=None) -> min:
    """
        Computes a tree decomposition using htd and then measures
        the resulting treedepth.
        (https://github.com/mabseher/htd).
    """
    import sys

    width, tree, bags = tw_pace(graph, logger, "htd_pace16", timeout)
    n = len(graph)

    if width == None:
        return None

    oldlimit = sys.getrecursionlimit()
    sys.setrecursionlimit(15000)
    logger.info("Recursive transformation of tree decomp of with {} with {} nodes".format(width, len(tree)))
    td = _td_from_tw(tree, bags, n)
    sys.setrecursionlimit(oldlimit)
    return td

def _td_from_tw(tree, bags, n):
    e = _find_separator_edge(tree, bags, n)
    sep = bags[e[0]] & bags[e[1]] 

    # Remove separator from bag
    for u in bags:
        bags[u] = bags[u] - sep
    tree.remove_edge(*e)

    res = []
    for comp in tree.get_components():
        if len(comp) == 1:
            res.append(1)
            continue
        subtree = tree.subgraph(comp)
        subtd = _td_from_tw(subtree, bags, len(comp))
        res.append(subtd)

    return len(sep) + max(res)

def _find_separator_edge(tree, bags, n):
    from operator import itemgetter
    def dfs(parent, current, tree, bags, res):
        nonlocal n
        if tree.degree(current) == 1:
            res[(parent,current)] = len(bags[current] - bags[parent]) 
            res[(current,parent)] = n - len(bags[current] - bags[parent]) 
            return 

        nodes_below = 0
        for child in tree.neighbours(current):
            if child == parent:
                continue
            dfs(current, child, tree, bags, res)
            nodes_below += res[(current,child)]

        res[(parent,current)] = nodes_below + len(bags[current] - bags[parent]) 
        res[(current,parent)] = n - res[(parent,current)]

    if len(tree) == 2:
        u,v = list(tree.edges())[0]
        return (u,v)

    # Pick a root (max-deg node to avoid picking a leaf)
    r, _ = max([(v,tree.degree(v)) for v in tree], key=itemgetter(1))
    labelling = {}
    dfs(r, r, tree, bags, labelling)

    previous, current = None, r
    while True:
        cands = [(u,labelling[(current,u)]) for u in tree.neighbours(current)]
        _next, _ = max(cands, key=itemgetter(1))
        if _next == previous:
            break
        previous, current = current, _next
    return (previous, current)

def compute_td_oel(graph, logger, timeout=None) -> min:
    """
        Computes the treedepth using separator heuristics.
    """
    import os, subprocess

    with open('tmp.dimacs', 'w') as filebuf:
        # The td program only works for connected graphs, so we add an
        # apex vertex and subtract one from the result.
        num_edges = graph.num_edges() + len(graph) # Accounts for apex
        num_vertices = len(graph) + 1 # Accounts for apex
        filebuf.write("p edge {} {}\n".format(num_vertices, num_edges))

        apex = 0
        for u,v in graph.edges():
            filebuf.write("e {} {}\n".format(u+1,v+1)) # The dimacs format is 1-based.
            apex = max(apex, u)
            apex = max(apex, v)
        apex += 1

        for u in graph:
            filebuf.write("e {} {}\n".format(apex+1, u+1))

    # Probably should use flag "-s" for large graphs.
    if not os.path.exists("statistics/td/td-bs"):
        print("Could not find statistics/td/td-bs. Did you run make inside statistics/td/?")
        sys.exit()

    try:
        process = subprocess.run(["statistics/td/td-bs", "-s", "-p", "-n", "tmp.dimacs"], stderr=subprocess.PIPE, timeout=timeout )
        output = str(process.stderr)

        td = int(output.split(",")[2].split(":")[1].strip())
        td -= 1 # Subtract apex contribution
    except:
        td = None
        
    # os.remove('tmp.dimacs')

    return td