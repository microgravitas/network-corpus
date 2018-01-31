#! /usr/bin/env python3

import sys, glob, os
import gzip
import logging
from collections import defaultdict

import statistics
from statistics import *

from dtf.graph import Graph

import textwrap
import inspect
import argparse

from tinydb import TinyDB, Query
import json

from compute_statistic import list_networks, network_file

if __name__ == '__main__':
    db = TinyDB('statistics.json', sort_keys=True, indent=4)
    
    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('networks', nargs='*')

    args = parser.parse_args()

    networks = args.networks
    if 'all' in networks:
        networks = list_networks()

    for name in networks:
        Network = Query()
        doc = db.get(Network.name == name)
        print("{} ({} nodes, {} edges)".format(name, doc['n'], doc['m']))
        for k in sorted(doc):
            if k in ['n', 'm', 'name']:
                continue
            if isinstance(doc[k], float):
                print(" {:>8} : {:.2f}".format(k, doc[k]))
            else:
                print(" {:>8} : {}".format(k, doc[k]))
