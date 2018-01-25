#! /usr/bin/env python3

import sys, glob, os
import gzip
sys.path.insert(0, "/home/ubik/git/programs/augmental")

import statistics
from statistics import *

from dtf.graph import Graph

import textwrap
import inspect
import argparse

from tinydb import TinyDB, Query

db = TinyDB('statistics.json', sort_keys=True, indent=4)

def list_networks():
    for f in glob.glob("networks/*.txt.gz"):
        name = os.path.basename(f)[:-7]
        yield name

def load_network(name):
    res = Graph()
    with gzip.open("networks/{}.txt.gz".format(name), 'r') as filebuf:
        for l in filebuf:
            u, v = l.decode().split()
            u, v = int(u), int(v)
            if u == v:
                continue
            res.add_edge(u,v)
    return res

def replace(newvalue, oldvalue):
    return newvalue

# Load statistic functions
stat_dict = {}
update_action = {}
for name, module in inspect.getmembers(statistics, inspect.ismodule):
    if name not in statistics.__all__:
        continue

    all_functions = inspect.getmembers(module, inspect.isfunction)
    stat_functions  = filter(lambda f: f[0].startswith("compute_"), all_functions)
    for name, f in stat_functions:
        name = name[len("compute_"):] 
        stat_dict[name] = f
        update_action[name] = replace
        if 'return' in f.__annotations__:
            update_action[name] = f.__annotations__['return']

    # stat_functions = [ (name[len("compute_"):], f) for name, f in stat_functions]
    # stat_dict.update(stat_functions)

if len(sys.argv) == 1:
    print("Available statistics: ")
    for name, f in sorted(stat_dict.items()):
        prefix = "{:>8} [{:^7}] ".format(name, update_action[name].__name__)
        wrapper = textwrap.TextWrapper(initial_indent=prefix, width=70,
                               subsequent_indent=' '*21)
        if f.__doc__:
            text = ' '.join(f.__doc__.split()) # Equalize whitespace
            print("  "+wrapper.fill(text))
        else:
            print("  "+wrapper.fill("(No documentation)"))
    sys.exit()


parser = argparse.ArgumentParser()
parser.add_argument('statistics', nargs='*')
parser.add_argument('--force', action='store_true')
parser.add_argument('--test', action='store_true')
parser.add_argument('--timeout', type=int)

args = parser.parse_args()

stats = args.statistics
if 'all' in stats:
    stats = stat_dict.keys()

for stat_name in stats:
    print("Computing {}".format(stat_name))
    if stat_name not in stat_dict:
        print("Error: '{}' is not a known statistic.".format(stat_name))
        print("  Call this programm without argments for a list of supported statistic.")
        continue

    stat_func = stat_dict[stat_name]
    action = update_action[stat_name]
    for name in list_networks():
        Network = Query()
        doc = db.get(Network.name == name)
        res = None
        if doc and stat_name in doc:
            res = doc[stat_name]

        if res and not args.force:
            continue

        g = load_network(name)
        value = stat_func(g, timeout=args.timeout)

        if value == None:
            print("Computing {} failed or timed out on network {}".format(stat_name, name))
            continue

        res = value if not res else action(value, res)

        if isinstance(res, int):
            print("  {} {}".format(name, res))
        else:
            print("  {} {:.2f}".format(name, res))
        
        if args.test:
            continue
        db.upsert({'name': name, stat_name: res}, Network.name == name)