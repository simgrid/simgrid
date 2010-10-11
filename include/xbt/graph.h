/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPH_H
#define _XBT_GRAPH_H
#include "xbt/misc.h"           /* SG_BEGIN_DECL */
#include "xbt/dynar.h"
SG_BEGIN_DECL()

  /** @addtogroup XBT_graph
   *  @brief A graph data type with several interesting algorithms
   *
   * @{
   */
typedef struct xbt_node *xbt_node_t;
typedef struct xbt_edge *xbt_edge_t;
typedef struct xbt_graph *xbt_graph_t;

/* API */
XBT_PUBLIC(xbt_graph_t) xbt_graph_new_graph(unsigned short int directed,
                                            void *data);
XBT_PUBLIC(xbt_node_t) xbt_graph_new_node(xbt_graph_t g, void *data);
XBT_PUBLIC(xbt_edge_t) xbt_graph_new_edge(xbt_graph_t g, xbt_node_t src,
                                          xbt_node_t dst, void *data);
XBT_PUBLIC(void *) xbt_graph_node_get_data(xbt_node_t node);
XBT_PUBLIC(void) xbt_graph_node_set_data(xbt_node_t node, void *data);
XBT_PUBLIC(void *) xbt_graph_edge_get_data(xbt_edge_t edge);
XBT_PUBLIC(void) xbt_graph_edge_set_data(xbt_edge_t edge, void *data);

XBT_PUBLIC(xbt_edge_t) xbt_graph_get_edge(xbt_graph_t g, xbt_node_t src,
                                          xbt_node_t dst);

XBT_PUBLIC(void) xbt_graph_edge_set_length(xbt_edge_t e, double length);
XBT_PUBLIC(double) xbt_graph_edge_get_length(xbt_edge_t e);
XBT_PUBLIC(double *) xbt_graph_get_length_matrix(xbt_graph_t g);

XBT_PUBLIC(void) xbt_graph_free_node(xbt_graph_t g, xbt_node_t n,
                                     void_f_pvoid_t node_free_function,
                                     void_f_pvoid_t edge_free_function);
XBT_PUBLIC(void) xbt_graph_free_edge(xbt_graph_t g, xbt_edge_t e,
                                     void_f_pvoid_t free_function);
XBT_PUBLIC(void) xbt_graph_free_graph(xbt_graph_t g,
                                      void_f_pvoid_t node_free_function,
                                      void_f_pvoid_t edge_free_function,
                                      void_f_pvoid_t graph_free_function);

XBT_PUBLIC(int) __xbt_find_in_dynar(xbt_dynar_t dynar, void *p);

XBT_PUBLIC(xbt_dynar_t) xbt_graph_get_nodes(xbt_graph_t g);
XBT_PUBLIC(xbt_dynar_t) xbt_graph_get_edges(xbt_graph_t g);
XBT_PUBLIC(xbt_dynar_t) xbt_graph_node_get_outedges(xbt_node_t n);
XBT_PUBLIC(xbt_node_t) xbt_graph_edge_get_source(xbt_edge_t e);
XBT_PUBLIC(xbt_node_t) xbt_graph_edge_get_target(xbt_edge_t e);
XBT_PUBLIC(xbt_graph_t) xbt_graph_read(const char *filename, void
                                       *(node_label_and_data) (xbt_node_t,
                                                               const char
                                                               *,
                                                               const char
                                                               *), void
                                       *(edge_label_and_data) (xbt_edge_t,
                                                               const char
                                                               *,
                                                               const char
                                                               *)
    );

XBT_PUBLIC(void) xbt_graph_export_graphviz(xbt_graph_t g,
                                           const char *filename, const char
                                           *(node_name) (xbt_node_t), const char
                                           *(edge_name) (xbt_edge_t));
XBT_PUBLIC(void) xbt_graph_export_graphxml(xbt_graph_t g,
                                           const char *filename, const char
                                           *(node_name) (xbt_node_t), const char
                                           *(edge_name) (xbt_edge_t), const char
                                           *(node_data_print) (void *), const char
                                           *(edge_data_print) (void *));

/* Not implemented yet ! */
/* void *xbt_graph_to_array(xbt_graph_t g);  */
XBT_PUBLIC(xbt_node_t *) xbt_graph_shortest_paths(xbt_graph_t g);



/** @brief transforms the network structure of a directed acyclic graph given into a linear structure
    @return: an array containing the nodes of the graph sorted in order reverse to the path of exploration
            if a cycle is detected an exception is raised
  */

XBT_PUBLIC(xbt_node_t *) xbt_graph_topo_sort(xbt_graph_t g);

XBT_PUBLIC(xbt_edge_t *) xbt_graph_spanning_tree_prim(xbt_graph_t g);




/** Convenient for loop : g is a graph, n a node, e an edge, b a bucket and i an item **/

/* #define xbt_graph_foreachInNeighbor(v,n,i)            \ */
/*    for(i=xbt_fifo_get_first_item((v)->in);              \ */
/*      ((i)?(n=((xbt_edge_t)((xbt_fifo_get_item_content(i)) */
/* )->src):(NULL));\ */
/*        i=xbt_fifo_get_next_item(i)) */
/* #define xbt_graph_foreachOutNeighbor(v,n,i)           \ */
/*    for(i=xbt_fifo_get_first_item((v)->out);             \ */
/*      ((i)?(n=((xbt_edge_t)(xbt_fifo_get_item_content(i)))->dst):(NULL));\ */
/*        i=xbt_fifo_get_next_item(i)) */



SG_END_DECL()
#endif                          /* _XBT_GRAPH_H */
/** @} */
