
def compute_td(graph, **kwargs):
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
        process = subprocess.run(["statistics/td/td-bs", "-s", "-p", "-n", "tmp.dimacs"], stderr=subprocess.PIPE, )
        output = str(process.stderr)

        td = int(output.split(",")[2].split(":")[1].strip())
        td -= 1 # Subtract apex contribution
    except:
        td = None
        
    # os.remove('tmp.dimacs')

    return td