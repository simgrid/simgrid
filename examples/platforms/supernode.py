#! /usr/bin/env python3

# Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.


# This script takes as input a C++ platform file, compiles it, then dumps the
# routing graph as a CSV and generates an SVG image.
# The layout should be alright for any platform file, but the colors are very
# ad-hoc for file supernode.cpp : do not hesitate to adapt this script to your needs.

import sys
import subprocess
import pandas
import matplotlib.pyplot as plt
import networkx as nx
try:
    from palettable.colorbrewer.qualitative import Set1_9
    colors = Set1_9.hex_colors
except ImportError:
    print('Warning: could not import palettable, will use a default palette.')
    colors = [None]*10


def run_command(cmd):
    print(cmd)
    subprocess.run(cmd.split(), capture_output=True, check=True)


def compile_platform(platform_cpp):
    platform_so = platform_cpp.replace('.cpp', '.so')
    cmd = f'g++ -g -fPIC -shared -o {platform_so} {platform_cpp} -lsimgrid'
    run_command(cmd)
    return platform_so


def dump_csv(platform_so):
    platform_csv = platform_so.replace('.so', '.csv')
    cmd = f'graphicator {platform_so} {platform_csv}'
    run_command(cmd)
    return platform_csv


def load_graph(platform_csv):
    edges = pandas.read_csv(platform_csv)
    G = nx.Graph()
    G.add_edges_from([e for _, e in edges.drop_duplicates().iterrows()])
    print(f'Loaded a graph with {len(G)} vertices with {len(G.edges)} edges')
    return G


def plot_graph(graph, label=False, groups=[]):
    # First, we compute the graph layout, i.e. the position of the nodes.
    # The neato algorithm from graphviz is nicer, but this is an extra-dependency.
    # The spring_layout is also not too bad.
    try:
        pos = nx.nx_agraph.graphviz_layout(graph, 'neato')
    except ImportError:
        print('Warning: could not import pygraphviz, will use another layout algorithm.')
        pos = nx.spring_layout(graph, k=0.5, iterations=1000, seed=42)
    plt.figure(figsize=(20, 15))
    plt.axis('off')
    all_nodes = set(graph)
    # We then iterate on all the specified groups, to plot each of them in the right color.
    # Note that the order of the groups is important here, as we are looking at substrings in the node names.
    for i, grp in enumerate(groups):
        nodes = {u for u in all_nodes if grp in u}
        all_nodes -= nodes
        nx.draw_networkx_nodes(graph, pos, nodelist=nodes, node_size=50, node_color=colors[i], label=grp.replace('_', ''))
    nx.draw_networkx_nodes(graph, pos, nodelist=all_nodes, node_size=50, node_color=colors[-1], label='unknown')
    # Finally we draw the edges, the (optional) labels, and the legend.
    nx.draw_networkx_edges(graph, pos, alpha=0.3)
    if label:
        nx.draw_networkx_labels(graph, pos)
    plt.legend(scatterpoints = 1)


def generate_svg(platform_csv):
    graph = load_graph(platform_csv)
    plot_graph(graph, label=False, groups=['router', 'link', 'cpu', '_node', 'supernode', 'cluster'])
    img = platform_csv.replace('.csv', '.svg')
    plt.savefig(img)
    print(f'Generated file {img}')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(f'Syntax: {sys.argv[0]} platform.cpp')
    platform_cpp = sys.argv[1]
    platform_so = compile_platform(platform_cpp)
    platform_csv = dump_csv(platform_so)
    generate_svg(platform_csv)
