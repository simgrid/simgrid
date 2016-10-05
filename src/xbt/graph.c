/* a generic graph library.                                                 */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/graph.h"
#include "graph_private.h"
#include "xbt/dict.h"
#include "xbt/heap.h"
#include "xbt/str.h"
#include "xbt/file.h"

#include <errno.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_graph, xbt, "Graph");

/** @brief Constructor
 *  @return a new graph
 */
xbt_graph_t xbt_graph_new_graph(unsigned short int directed, void *data)
{
  xbt_graph_t graph = NULL;
  graph = xbt_new0(struct xbt_graph, 1);
  graph->directed = directed;
  graph->data = data;
  graph->nodes = xbt_dynar_new(sizeof(xbt_node_t), NULL);
  graph->edges = xbt_dynar_new(sizeof(xbt_edge_t), NULL);

  return graph;
}

/** @brief add a node to the given graph */
xbt_node_t xbt_graph_new_node(xbt_graph_t g, void *data)
{
  xbt_node_t node = NULL;
  node = xbt_new0(struct xbt_node, 1);
  node->data = data;
  if (g->directed)
    /* only the "out" field is used */
    node->in = xbt_dynar_new(sizeof(xbt_edge_t), NULL);

  node->out = xbt_dynar_new(sizeof(xbt_edge_t), NULL);
  node->position_x = -1.0;
  node->position_y = -1.0;

  xbt_dynar_push(g->nodes, &node);

  return node;
}

/** @brief add an edge to the given graph */
xbt_edge_t xbt_graph_new_edge(xbt_graph_t g, xbt_node_t src, xbt_node_t dst, void *data)
{
  xbt_edge_t edge = NULL;

  edge = xbt_new0(struct xbt_edge, 1);
  xbt_dynar_push(src->out, &edge);
  if (g->directed)
    xbt_dynar_push(dst->in, &edge);
  else                          /* only the "out" field is used */
    xbt_dynar_push(dst->out, &edge);

  edge->data = data;
  edge->src = src;
  edge->dst = dst;

  xbt_dynar_push(g->edges, &edge);

  return edge;
}

/** @brief Get the edge connecting src and dst */
xbt_edge_t xbt_graph_get_edge(xbt_graph_t g, xbt_node_t src, xbt_node_t dst)
{
  xbt_edge_t edge;
  unsigned int cursor;

  xbt_dynar_foreach(src->out, cursor, edge) {
    XBT_DEBUG("%p = %p--%p", edge, edge->src, edge->dst);
    if ((edge->src == src) && (edge->dst == dst))
      return edge;
  }
  if (!g->directed) {
    xbt_dynar_foreach(src->out, cursor, edge) {
      XBT_DEBUG("%p = %p--%p", edge, edge->src, edge->dst);
      if ((edge->dst == src) && (edge->src == dst))
        return edge;
    }
  }
  return NULL;
}

/** @brief Get the user data associated to a node */
void *xbt_graph_node_get_data(xbt_node_t node)
{
  return node->data;
}

/** @brief Set the user data associated to a node */
void xbt_graph_node_set_data(xbt_node_t node, void *data)
{
  node->data = data;
}

/** @brief Get the user data associated to a edge */
void *xbt_graph_edge_get_data(xbt_edge_t edge)
{
  return edge->data;
}

/** @brief Set the user data associated to a edge */
void xbt_graph_edge_set_data(xbt_edge_t edge, void *data)
{
  edge->data = data;
}

/** @brief Destructor
 *  @param g: poor victim
 *  @param node_free_function: function to use to free data associated to each node
 *  @param edge_free_function: function to use to free data associated to each edge
 *  @param graph_free_function: function to use to free data associated to g
 *
 * Free the graph structure.
 */
void xbt_graph_free_graph(xbt_graph_t g, void_f_pvoid_t node_free_function, void_f_pvoid_t edge_free_function,
                          void_f_pvoid_t graph_free_function)
{
  unsigned int cursor;
  xbt_node_t node;
  xbt_edge_t edge;

  xbt_dynar_foreach(g->edges, cursor, edge) {
    if (edge_free_function)
      edge_free_function(edge->data);
    free(edge);
  }
  xbt_dynar_free(&(g->edges));

  xbt_dynar_foreach(g->nodes, cursor, node) {
    xbt_dynar_free(&(node->out));
    xbt_dynar_free(&(node->in));
    if (node_free_function)
      node_free_function(node->data);
    free(node);
  }
  xbt_dynar_free(&(g->nodes));

  if (graph_free_function)
    graph_free_function(g->data);
  free(g);
}

/** @brief Retrieve the graph's nodes as a dynar */
xbt_dynar_t xbt_graph_get_nodes(xbt_graph_t g)
{
  return g->nodes;
}

/** @brief Retrieve the graph's edges as a dynar */
xbt_dynar_t xbt_graph_get_edges(xbt_graph_t g)
{
  return g->edges;
}

/** @brief Retrieve the node at the source of the given edge */
xbt_node_t xbt_graph_edge_get_source(xbt_edge_t e)
{
  return e->src;
}

/** @brief Retrieve the node being the target of the given edge */
xbt_node_t xbt_graph_edge_get_target(xbt_edge_t e)
{
  return e->dst;
}

/** @brief Retrieve the outgoing edges of the given node */
xbt_dynar_t xbt_graph_node_get_outedges(xbt_node_t n)
{
  return n->out;
}

/** @brief Set the weight of the given edge */
void xbt_graph_edge_set_length(xbt_edge_t e, double length)
{
  e->length = length;

}

/** @brief Get the length of a edge */
double xbt_graph_edge_get_length(xbt_edge_t edge)
{
  return edge->length;
}

/** @brief Floyd-Warshall algorithm for shortest path finding
 *
 * From wikipedia:
 *
 * The Floyd–Warshall algorithm takes as input an adjacency matrix representation of a weighted, directed graph (V, E).
 * The weight of a path between two vertices is the sum of the weights of the edges along that path. The edges E of the
 * graph may have negative weights, but the graph must not have any negative weight cycles. The algorithm computes, for
 * each pair of vertices, the minimum weight among all paths between the two vertices. The running time complexity is
 * Θ(|V|3).
 */
void xbt_floyd_algorithm(xbt_graph_t g, double *adj, double *d, xbt_node_t * p)
{
  unsigned long i, j, k;
  unsigned long n;
  n = xbt_dynar_length(g->nodes);

# define D(u,v) d[(u)*n+(v)]
# define P(u,v) p[(u)*n+(v)]

  for (i = 0; i < n * n; i++) {
    d[i] = adj[i];
  }

  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      if (D(i, j) != -1) {
        P(i, j) = *((xbt_node_t *) xbt_dynar_get_ptr(g->nodes, i));
      }
    }
  }

  for (k = 0; k < n; k++) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        if ((D(i, k) != -1) && (D(k, j) != -1)) {
          if ((D(i, j) == -1) || (D(i, j) > D(i, k) + D(k, j))) {
            D(i, j) = D(i, k) + D(k, j);
            P(i, j) = P(k, j);
          }
        }
      }
    }
  }
# undef P
# undef D
}

/** @brief Export the given graph in the GraphViz formatting for visualization */
void xbt_graph_export_graphviz(xbt_graph_t g, const char *filename, const char *(node_name) (xbt_node_t),
                               const char *(edge_name) (xbt_edge_t))
{
  unsigned int cursor = 0;
  xbt_node_t node = NULL;
  xbt_edge_t edge = NULL;
  FILE *file = NULL;
  const char *name = NULL;

  file = fopen(filename, "w");
  xbt_assert(file, "Failed to open %s \n", filename);

  if (g->directed)
    fprintf(file, "digraph test {\n");
  else
    fprintf(file, "graph test {\n");

  fprintf(file, "  graph [overlap=scale]\n");

  fprintf(file, "  node [shape=box, style=filled]\n");
  fprintf(file, "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");

  xbt_dynar_foreach(g->nodes, cursor, node) {
    if (node_name){
      fprintf(file, "  \"%s\";\n", node_name(node));
    }else{
      fprintf(file, "  \"%p\";\n", node);
    }
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    const char *c;
    const char *c_dir = "->";
    const char *c_ndir = "--";
    if (g->directed){
      c = c_dir;
    }else{
      c = c_ndir;
    }
    const char *src_name, *dst_name;
    if (node_name){
      src_name = node_name(edge->src);
      dst_name = node_name(edge->dst);
      fprintf(file, "  \"%s\" %s \"%s\"", src_name, c, dst_name);
    }else{
      fprintf(file, "  \"%p\" %s \"%p\"", edge->src, c, edge->dst);
    }

    if ((edge_name) && ((name = edge_name(edge))))
      fprintf(file, "[label=\"%s\"]", name);
    fprintf(file, ";\n");
  }
  fprintf(file, "}\n");
  fclose(file);
}
