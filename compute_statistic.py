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

from networks import list_networks, network_file, load_network

def replace(newvalue, oldvalue):
    return newvalue

if __name__ == '__main__':
    db = TinyDB('statistics.json', sort_keys=True, indent=4)

    # Load statistic functions
    stat_dict = defaultdict(dict)
    update_action = defaultdict(dict)
    for name, module in inspect.getmembers(statistics, inspect.ismodule):
        if name not in statistics.__all__:
            continue

        all_functions = inspect.getmembers(module, inspect.isfunction)
        stat_functions  = filter(lambda f: f[0].startswith("compute_"), all_functions)
        for name, f in stat_functions:
            _, name, *variant = name.split("_")
            varname = "" # This is the 'default' variant
            if variant:
                varname = "_".join(variant)

            stat_dict[name][varname] = f
            update_action[name][varname] = replace
            if 'return' in f.__annotations__:
                update_action[name][varname] = f.__annotations__['return']

    if len(sys.argv) == 1:
        print("Available statistics: ")
        for name, variants in sorted(stat_dict.items()):
            prefix = "{:>8} ".format(name)
            for varname, f in sorted(variants.items()):
                print(prefix, end='')
                prefix = " "*len(prefix)
                varprefix = "{:10} [{:^7}] ".format(varname, update_action[name][varname].__name__)
                if f.__doc__:
                    text = ' '.join(f.__doc__.split()) # Equalize whitespace
                    print(varprefix+text)
                else:
                    print(varprefix+"(No documentation)")

    # Set up logger
    logger = logging.getLogger('compute_statistic')
    hdlr = logging.FileHandler('compute_statistic.log')
    formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
    hdlr.setFormatter(formatter)
    logger.addHandler(hdlr) 
    logger.setLevel(logging.INFO)

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('statistics', nargs='*')
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--test', action='store_true')
    parser.add_argument('--timeout', type=int)
    parser.add_argument('--networks', nargs='*')

    args = parser.parse_args()

    stats = args.statistics
    if 'all' in stats:
        stats = stat_dict.keys()
    elif 'fast' in stats:
        stats = ['m','n','avgdeg','maxdeg']

    for statname in stats:
        varname = ""
        if ":" in statname:
            statname, varname = statname.split(":")

        if varname not in stat_dict[statname]:
            if varname != "":
                print("Unknown method {}, reverting to default.".format(varname))
            if "" in stat_dict[statname]:
                varname = ""
            else:
                varname = list(stat_dict[statname].keys())[0]

        print("Computing {} ({})".format(statname, "default method" if len(varname) == 0 else "using method {}".format(varname)))
        if statname not in stat_dict:
            print("Error: '{}' is not a known statistic.".format(statname))
            print("  Call this programm without argments for a list of supported statistic.")
            continue

        stat_func = stat_dict[statname][varname]
        action = update_action[statname][varname]
        
        if args.networks:
            networks = args.networks
        else:
            networks = list_networks()

        for name in networks:
            Network = Query()
            doc = db.get(Network.name == name)
            res = None
            if doc and statname in doc:
                res = doc[statname]

            if res and not args.force:
                continue

            logger.info("Computing {} on network {} with timeout {}".format(statname, name, args.timeout))
            try:
                g = load_network(name)
            except FileNotFoundError:
                logger.error("Network file '{}' could not be found".format(network_file(name)))
                print("Network file '{}' could not be found".format(network_file(name)))
                continue

            value = stat_func(g, network_file(name), logger, args.timeout)

            if value == None:
                print("Computing {} failed or timed out on network {}".format(statname, name))
                continue

            res = value if not res else action(value, res)

            if isinstance(res, int):
                print("  {} {}".format(name, res))
            else:
                print("  {} {:.2f}".format(name, res))
            
            if args.test:
                continue
            db.upsert({'name': name, statname: res}, Network.name == name)