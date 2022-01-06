/* a generic graph library.                                                 */

/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/graph.h"
#include "xbt/dict.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_graph, xbt, "Graph");

/** @brief Constructor
 *  @return a new graph
 */
xbt_graph_t xbt_graph_new_graph(unsigned short int directed, void *data)
{
  xbt_graph_t graph = xbt_new0(struct xbt_graph, 1);
  graph->directed = directed;
  graph->data = data;
  graph->nodes = xbt_dynar_new(sizeof(xbt_node_t), NULL);
  graph->edges = xbt_dynar_new(sizeof(xbt_edge_t), NULL);

  return graph;
}

/** @brief add a node to the given graph */
xbt_node_t xbt_graph_new_node(const s_xbt_graph_t* g, void* data)
{
  xbt_node_t node= xbt_new0(struct xbt_node, 1);
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
xbt_edge_t xbt_graph_new_edge(const s_xbt_graph_t* g, xbt_node_t src, xbt_node_t dst, void* data)
{
  xbt_edge_t edge = xbt_new0(struct xbt_edge, 1);
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
xbt_edge_t xbt_graph_get_edge(const s_xbt_graph_t* g, const s_xbt_node_t* src, const s_xbt_node_t* dst)
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
void* xbt_graph_node_get_data(const s_xbt_node_t* node)
{
  return node->data;
}

/** @brief Set the user data associated to a node */
void xbt_graph_node_set_data(xbt_node_t node, void *data)
{
  node->data = data;
}

/** @brief Get the user data associated to an edge */
void* xbt_graph_edge_get_data(const s_xbt_edge_t* edge)
{
  return edge->data;
}

/** @brief Set the user data associated to an edge */
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
xbt_dynar_t xbt_graph_get_nodes(const s_xbt_graph_t* g)
{
  return g->nodes;
}

/** @brief Retrieve the graph's edges as a dynar */
xbt_dynar_t xbt_graph_get_edges(const s_xbt_graph_t* g)
{
  return g->edges;
}

/** @brief Retrieve the node at the source of the given edge */
xbt_node_t xbt_graph_edge_get_source(const s_xbt_edge_t* e)
{
  return e->src;
}

/** @brief Retrieve the node being the target of the given edge */
xbt_node_t xbt_graph_edge_get_target(const s_xbt_edge_t* e)
{
  return e->dst;
}

/** @brief Retrieve the outgoing edges of the given node */
xbt_dynar_t xbt_graph_node_get_outedges(const s_xbt_node_t* n)
{
  return n->out;
}
