#! /usr/bin/env python3 

import sys, os, re

import math
from graph.graph import Graph
from graph.graphformats import load_graph

import gzip
import subprocess

def sorted_nicely( l ): 
    convert = lambda text: int(text) if text.isdigit() else text 
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ] 
    return sorted(l, key = alphanum_key)

def normalize(g):
    if all( isinstance(u,int) for u in g ):
        return g
    # If the nodes are simply numbers decorated with some prefix,
    # this sorting will most likely preserve the correct order.
    nodemap = dict(zip(sorted_nicely(g), range(len(g))))
    res = Graph()
    for u,v in g.edges():
        res.add_edge(nodemap[u], nodemap[v])
    return res

def import_network(networkfile):
    g = load_graph(networkfile)
    networkname = os.path.splitext(os.path.basename(networkfile))[0]
    print("Importing network {}".format(networkname))

    g = normalize(g)

    infofile = os.path.splitext(networkfile)[0] + ".info"

    networkfile = "networks/{}.txt.gz".format(networkname)
    if os.path.exists(networkfile):
        print("Network file {} already exists, skipping.".format(networkfile))
        return

    if not os.path.exists(infofile):
        print("No .info file found. Please provide a quick description.")

        lines = []
        while True:
            line = input()
            if line:
                lines.append(line)
            else:
                break
        infotext = '\n'.join(lines)
    else:
        infotext = open(infofile, 'r').read()

    infofile = "networks/{}.info".format(networkname)

    with gzip.GzipFile(networkfile, mode='w') as filebuf:
        for u,v in g.edges():
            filebuf.write("{} {}\n".format(u,v).encode())

    with open(infofile, 'w') as filebuf:
        filebuf.write(infotext)    

    gitcmd = ['git', 'add', 'networks/{0}.txt.gz'.format(networkname), 'networks/{0}.info'.format(networkname)]
    print(' '.join(gitcmd))
    subprocess.run(gitcmd)
    
    gitcmd[1] = 'commit'
    gitcmd.append('-m "Added network {0}"'.format(networkname))
    print(' '.join(gitcmd))
    subprocess.run(gitcmd)

if __name__ == '__main__':
    for networkfile in sys.argv[1:]:
        import_network(networkfile)
    


