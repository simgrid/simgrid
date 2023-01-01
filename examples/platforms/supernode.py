#! /usr/bin/env python3

# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.


# This script takes as input a C++ platform file, compiles it, then dumps the
# routing graph as a CSV and generates an image.
# The layout should be alright for any platform file, but the colors are very
# ad-hoc for file supernode.cpp : do not hesitate to adapt this script to your needs.
# An option is provided to "simplify" the graph by removing the link vertices. It assumes that these vertices have
# "link" in their name.

import sys
import subprocess
import pandas
import matplotlib.pyplot as plt
import networkx as nx
import argparse
import tempfile
import os

try:
    from palettable.colorbrewer.qualitative import Set1_9
    colors = Set1_9.hex_colors
except ImportError:
    print('Warning: could not import palettable, will use a default palette.')
    colors = [None]*10


def run_command(cmd):
    print(cmd)
    proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    _stdout, stderr = proc.communicate()
    if proc.returncode != 0:
        sys.exit(f'Command failed:\n{stderr.decode()}')


def compile_platform(platform_cpp, platform_so):
    cmd = f'g++ -g -fPIC -shared -o {platform_so} {platform_cpp} -lsimgrid'
    run_command(cmd)


def dump_csv(platform_so, platform_csv):
    cmd = f'graphicator {platform_so} {platform_csv}'
    run_command(cmd)


def merge_updown(graph):
    '''
    Merge all the UP and DOWN links.
    '''
    graph2 = graph.copy()
    downlinks = [v for v in graph if 'DOWN' in v]
    mapping = {}
    for down in downlinks:
        up = down.replace('DOWN', 'UP')
        graph2 = nx.contracted_nodes(graph2, down, up)
        mapping[down] = down.replace('_DOWN', '')
    return nx.relabel_nodes(graph2, mapping)


def contract_links(graph):
    '''
    Remove all the 'link' vertices from the graph to directly connect the nodes.
    Note: it assumes that link vertices have the "link" string in their name.
    '''
    links = [v for v in graph if 'link' in v]
    new_edges = []
    for v in links:
        neigh = [u for u in graph.neighbors(v) if 'link' not in u]  # with Floyd zones, we have links connected to links
        assert len(neigh) == 2
        new_edges.append(neigh)
    # Adding edges from graph that have no links
    for u, v in graph.edges:
        if 'link' not in u and 'link' not in v:
            new_edges.append((u, v))
    return nx.from_edgelist(new_edges)


def load_graph(platform_csv, simplify_graph):
    edges = pandas.read_csv(platform_csv)
    graph = nx.Graph()
    graph.add_edges_from([e for _, e in edges.drop_duplicates().iterrows()])
    print(f'Loaded a graph with {len(graph)} vertices with {len(graph.edges)} edges')
    if simplify_graph:
        graph = contract_links(merge_updown(graph))
        print(f'Simplified the graph, it now has {len(graph)} vertices with {len(graph.edges)} edges')
    return graph


def plot_graph(graph, label=False, groups=None):
    if groups is None:
        groups = []
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
    plt.legend(scatterpoints=1)


def generate_svg(platform_csv, output_file, simplify_graph):
    graph = load_graph(platform_csv, simplify_graph)
    plot_graph(graph, label=False, groups=['router', 'link', 'cpu', '_node', 'supernode', 'cluster'])
    plt.savefig(output_file)
    print(f'Generated file {output_file}')


def main():
    parser = argparse.ArgumentParser(description='Visualization of topologies for SimGrid C++ platforms')
    parser.add_argument('input', type=str, help='SimGrid C++ platform file name (input)')
    parser.add_argument('output', type=str, help='File name of the output image')
    parser.add_argument('--simplify', action='store_true', help='Simplify the topology (removing link vertices)')
    args = parser.parse_args()
    if not args.input.endswith('.cpp'):
        parser.error(f'SimGrid platform must be a C++ file (with .cpp extension), got {args.input}')
    if not os.path.isfile(args.input):
        parser.error(f'File {args.input} not found')
    output_dir = os.path.dirname(args.output)
    if output_dir != '' and not os.path.isdir(output_dir):
        parser.error(f'Not a directory: {output_dir}')
    with tempfile.TemporaryDirectory() as tmpdirname:
        platform_cpp = args.input
        platform_csv = os.path.join(tmpdirname, 'platform.csv')
        platform_so = os.path.join(tmpdirname, 'platform.so')
        compile_platform(platform_cpp, platform_so)
        dump_csv(platform_so, platform_csv)
        generate_svg(platform_csv, args.output, args.simplify)


if __name__ == '__main__':
    main()
