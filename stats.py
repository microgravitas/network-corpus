#! /usr/bin/env python3

import sys, glob, os
import gzip
import logging
from collections import defaultdict

import statistics
from statistics import *

from dtf.graph import Graph

import textwrap
import argparse

from fuzzywuzzy import process
from tinydb import TinyDB, Query
import json

from networks import list_networks, network_file, network_info

class ColumnPrinter(object):
    def __init__(self, out=sys.stdout, left=39, right=39, sep="  "):
        self.left_size, self.right_size = left, right
        self.format_str = "{{:{}}}".format(left) + sep + "{{:{}}}".format(right) + "\n"
        self.left_lines = []
        self.right_lines = []
        self.out = out

    def _split_lines(self, s):
        if s == None:
            yield ""
            return
        for l in s.split('\n'):
            yield str(l)

    def print_left(self, s=None):
        for l in self._split_lines(s):
            l = l[:min(len(l),self.left_size)]
            self.left_lines.append(l)
        self._print()

    def print_right(self, s=None):
        for l in self._split_lines(s):
            l = l[:min(len(l),self.right_size)]
            self.right_lines.append(l)
        self._print()

    def print(self, s=None):
        self.flush_columns()
        for l in self._split_lines(s):
            self.out.write(l)
            self.out.write('\n')

    def flush_columns(self):
        self._print()

        # One of left_lines/right_lines will be empty
        for l in self.left_lines:
            self.out.write(self.format_str.format(l, ""))
        for r in self.right_lines:
            self.out.write(self.format_str.format("",r))
        self.left_lines = []
        self.right_lines = []

    def _print(self):
        printed = 0
        for l, r in zip(self.left_lines, self.right_lines):
            self.out.write(self.format_str.format(l, r))
            printed += 1
        self.left_lines = self.left_lines[printed:]
        self.right_lines = self.right_lines[printed:]


def print_network(doc, printer=None):
    if printer:
        print_left = printer.print_left
        print_right = printer.print_right
        _print = printer.print
    else:
        print_left = print
        print_right = lambda s: None
        _print = print

    print_left("{} ({} nodes, {} edges)".format(name, doc['n'], doc['m']))
    for k in sorted(doc):
        if k in ['n', 'm', 'name']:
            continue
        if isinstance(doc[k], float):
            print_left(" {:>8} : {:.2f}".format(k, doc[k]))
        else:
            print_left(" {:>8} : {}".format(k, doc[k]))

    print_right(network_info(name))
    _print()

if __name__ == '__main__':
    db = TinyDB('statistics.json', sort_keys=True, indent=4)

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('networks', nargs='*')
    parser.add_argument('-v', '--verbose', action='store_true' )

    args = parser.parse_args()

    names = [res['name'] for res in db.search(Query().name.exists())]

    networks = args.networks
    if 'all' in networks:
        networks = list_networks()

    if args.verbose:
        printer = ColumnPrinter(left=30, right=70)
    else:
        printer = None

    for name in networks:
        Network = Query()
        doc = db.get(Network.name == name)
        if not doc:
            print("Unknown network: '{}'. Did you mean {}?'".format(name, process.extractOne(name, names)[0]))
            continue

        print_network(doc, printer)
