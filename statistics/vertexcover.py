
def compute_vc(graph, logger, timeout=None) -> min:
    """
        Computes a minimum vertex cover using the solver
        by T.Akiba and Y.Iwata (https://github.com/wata-orz/vertex_cover)
    
        Akiba, Takuya, and Yoichi Iwata. 
        "Branch-and-reduce exponential/fpt algorithms in practice: A case study of vertex cover." 
        Theoretical Computer Science 609 (2016): 211-225.
    """
    import os, sys, subprocess
    with open('tmp.txt', 'w') as filebuf:
        for u,v in graph.edges():
            filebuf.write("{} {}\n".format(u,v))

    # The solver outputs the important information to stderr
    res = None  
    try:
        proc = subprocess.Popen(["java", "-cp", "statistics/vc/solver/bin", "Main", "tmp.txt"], 
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        _, err = proc.communicate(timeout = timeout)

        for l in err.decode().split('\n'):
            if 'opt' in l:
                res = int(l.split(',')[0].split('=')[1].strip())
                break
    except subprocess.TimeoutExpired as e:
        pass

    return res