#! /usr/bin/env python3 

import sys, os

import math, argparse
from dtf.graph import Graph
from dtf.graphformats import load_graph

import gzip
import subprocess

from compute_statistic import list_networks, network_file

def network_size(name, max_size):
    res = Graph()
    filename = network_file(name)
    nodes = set()
    with gzip.open(filename, 'r') as filebuf:
        for l in filebuf:
            u, v = l.decode().split()
            u, v = int(u), int(v)
            if u == v:
                continue 
            nodes.add(u)
            nodes.add(v)
            if len(nodes) > max_size:
                return max_size
    return len(nodes)

if __name__ == '__main__':
    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('--n_min', type=float, default=0)
    parser.add_argument('--n_max', type=float, default=math.inf)

    args = parser.parse_args()
    for name in list_networks():
        n = network_size(name, args.n_max)
        if n >= args.n_min and n <= args.n_max:
            print(name)
    


