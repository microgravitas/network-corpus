
def compute_td(graph, **kwargs):
    """
        Computes the treedepth using separator heuristics.
    """
    import os, subprocess
    with open('tmp.dimacs', 'w') as filebuf:
        filebuf.write("p edge {} {}\n".format(len(graph), graph.num_edges()))
        for u,v in graph.edges():
            filebuf.write("e {} {}\n".format(u+1,v+1))

    # Probably should use flag "-s" for large graphs.
    try:
        process = subprocess.run(["statistics/td/td-bs", "-s", "-p", "-n", "tmp.dimacs"], stderr=subprocess.PIPE, )
        output = str(process.stderr)

        td = int(output.split(",")[2].split(":")[1].strip())
    except:
        td = None
        
    os.remove('tmp.dimacs')

    return td