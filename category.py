#! /usr/bin/env python3 

import sys, os, re

import math
from dtf.graph import Graph
from dtf.graphformats import load_graph

from tinydb import TinyDB, Query

from networks import list_networks, network_info

def list_categories():
    return ['social', 'transportation', 'web', 'economic', 'citation', 
            'collaboration', 'protein', 'genetic', 'communication', 'neural',
            'conflict', 'other']

def query_category():
    choices = {i: cat for i, cat in enumerate(list_categories(), start=1)}
    n = len(choices)
    rows = list(range(1,n+1))
    rows = list(zip(rows[:n//2], rows[(n+1)//2:]))
    
    for i,j in rows:
        print("{}) {:<16} {}) {}".format(i,choices[i], j, choices[j]))

    if n % 2 == 1:
        k = n // 2 + 1 
        print("{}) {}".format(k, choices[k]))

    inp = input("Choice: ")
    res = None
    try:
        inp = int(inp)
        res = choices[inp]
    except:
        pass
    return res


if __name__ == "__main__":
    db = TinyDB('statistics.json', sort_keys=True, indent=4)

    Network = Query()
    # res = db.search(~Network.category.exists())
    res = db.all()

    try:
        for entry in res:
            name = entry['name']
            category = entry['category'] if 'category' in entry else None
            print("Network: {} ({})".format(name, category))
            print('-'*80)

            text = network_info(name).split('\n')
            text = text[:min(12, len(text))]
            text = '\n'.join(text)

            print(text)
            print('-'*80)

            print("Enter a category. No entry skips, CTRL+C exits.")
            choice = query_category()

            if choice:
                db.upsert({'category': choice}, Network.name == name)

            os.system('clear')
    except KeyboardInterrupt:
        print("\n\nExiting.")
        
    db.close()

    # try:
    #     while True:
    #         print("Enter a category. No entry skips, CTRL+C exits.")
    #         choice = query_category()

    #         try:
    #             choice = int(choice)
    #         except ValueError:
    #             choice = None
    #         print("Your choice", choice)
    #         print("")
    # except KeyboardInterrupt:
    #     print("\n\nExiting.")
    #     sys.exit()

    # db.upsert({'name': name, statname: res}, Network.name == name)