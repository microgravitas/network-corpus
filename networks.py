#! /usr/bin/env python3 

import sys, os, glob

import math, argparse
from dtf.graph import Graph
from dtf.graphformats import load_graph

import gzip
import subprocess

def list_networks():
    networks = []
    for f in glob.glob("networks/*.txt.gz"):
        name = os.path.basename(f)[:-7]
        networks.append(name) # Just so we have some control over the order
    for name in reversed(networks):    
        yield name

def network_file(name):
    return "networks/{}.txt.gz".format(name)

def network_info(name):
    res = None
    with open("networks/{}.info".format(name), 'r') as filebuf:
        res = filebuf.read()
    return res

def load_network(name):
    res = Graph()
    filename = network_file(name)
    with gzip.open(filename, 'r') as filebuf:
        for l in filebuf:
            u, v = l.decode().split()
            u, v = int(u), int(v)
            if u == v:
                continue
            res.add_edge(u,v)
    return res

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
                return max_size+1
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
    


