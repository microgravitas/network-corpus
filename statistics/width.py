
def compute_tw_flowcutter(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using the FlowCutter heuristic
        (https://github.com/ben-strasser/flow-cutter-pace16).
    """
    return tw_pace(graph, logger, "flow_cutter_pace16", timeout)

def compute_tw_foxepstein(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using Fox-Epstein heuristic
        (https://github.com/elitheeli/2016-pace-challenge)
    """
    return tw_pace(graph, logger, "fox_epstein_pace16", timeout)


def compute_tw_htd(graph, logger, timeout=None) -> min:
    """ 
        Computes the treewidth using the FlowCutter heuristic
        (https://github.com/mabseher/htd).
    """
    return tw_pace(graph, logger, "htd_pace16", timeout)

def tw_pace(graph, logger, program, timeout=None) -> min:
    """ 
        Wrapper for all pace16 (pace17?) treewidth heuristics
    """
    import os, sys, subprocess
    import time

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
    for l in out.decode().split('\n'):
        if len(l) > 0 and l[0] == "s":
            # Solution like has format 's td [num-bags] [max bag size] [vertices in graph]'
            tokens = l.split()
            assert tokens[1] == 'td'
            width = int(tokens[3])
            break

    os.remove('tmp.gr')

    return width

def compute_td(graph, logger, timeout=None) -> min:
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