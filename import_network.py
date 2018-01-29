#! /usr/bin/env python3 

import sys, os

import math
from dtf.graph import Graph
from dtf.graphformats import load_graph

import gzip
import subprocess

def import_network(networkfile):
    g = load_graph(networkfile)
    networkname = os.path.splitext(os.path.basename(networkfile))[0]
    print("Importing network {}".format(networkname))

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
    


