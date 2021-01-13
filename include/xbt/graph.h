/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_GRAPH_H
#define XBT_GRAPH_H

#include <xbt/dynar.h>
#include <xbt/misc.h> /* SG_BEGIN_DECL */

SG_BEGIN_DECL

/** @addtogroup XBT_graph
 *  @brief A graph data type with several interesting algorithms
 *
 * @{
 */

typedef struct xbt_node *xbt_node_t;
typedef struct xbt_edge *xbt_edge_t;
typedef struct xbt_graph *xbt_graph_t;

/* Node structure */
/* Be careful of what you do with this structure */
typedef struct xbt_node {
  xbt_dynar_t out;
  xbt_dynar_t in;               /* not used when the graph is directed */
  double position_x;            /* positive value: negative means undefined */
  double position_y;            /* positive value: negative means undefined */
  void *data;                   /* user data */
} s_xbt_node_t;

/* edge structure */
/* Be careful of what you do with this structure */
typedef struct xbt_edge {
  xbt_node_t src;
  xbt_node_t dst;
  void *data;                   /* user data */
} s_xbt_edge_t;

/* Graph structure */
/* Be careful of what you do with this structure */
typedef struct xbt_graph {
  xbt_dynar_t nodes;
  xbt_dynar_t edges;
  unsigned short int directed;
  void *data;                   /* user data */
} s_xbt_graph_t;

/* API */
XBT_PUBLIC xbt_graph_t xbt_graph_new_graph(unsigned short int directed, void* data);
XBT_PUBLIC xbt_node_t xbt_graph_new_node(const s_xbt_graph_t* g, void* data);
XBT_PUBLIC xbt_edge_t xbt_graph_new_edge(const s_xbt_graph_t* g, xbt_node_t src, xbt_node_t dst, void* data);
XBT_PUBLIC void* xbt_graph_node_get_data(const s_xbt_node_t* node);
XBT_PUBLIC void xbt_graph_node_set_data(xbt_node_t node, void* data);
XBT_PUBLIC void* xbt_graph_edge_get_data(const s_xbt_edge_t* edge);
XBT_PUBLIC void xbt_graph_edge_set_data(xbt_edge_t edge, void* data);

XBT_PUBLIC xbt_edge_t xbt_graph_get_edge(const s_xbt_graph_t* g, const s_xbt_node_t* src, const s_xbt_node_t* dst);

XBT_PUBLIC void xbt_graph_free_graph(xbt_graph_t g, void_f_pvoid_t node_free_function,
                                     void_f_pvoid_t edge_free_function, void_f_pvoid_t graph_free_function);

XBT_PUBLIC xbt_dynar_t xbt_graph_get_nodes(const s_xbt_graph_t* g);
XBT_PUBLIC xbt_dynar_t xbt_graph_get_edges(const s_xbt_graph_t* g);
XBT_PUBLIC xbt_dynar_t xbt_graph_node_get_outedges(const s_xbt_node_t* n);
XBT_PUBLIC xbt_node_t xbt_graph_edge_get_source(const s_xbt_edge_t* e);
XBT_PUBLIC xbt_node_t xbt_graph_edge_get_target(const s_xbt_edge_t* e);

SG_END_DECL
#endif /* XBT_GRAPH_H */
/** @} */
