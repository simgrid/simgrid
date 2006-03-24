/* 	$Id$	 */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand. 
   All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPH_H
#define _XBT_GRAPH_H
#include "xbt/misc.h" /* SG_BEGIN_DECL */
#include "xbt/dynar.h"
SG_BEGIN_DECL()

typedef struct xbt_node *xbt_node_t;
typedef struct xbt_edge  *xbt_edge_t;
typedef struct xbt_graph  *xbt_graph_t;

/* API */
xbt_graph_t xbt_graph_new_graph(unsigned short int directed, void *data);
xbt_node_t xbt_graph_new_node(xbt_graph_t g, void *data);
xbt_edge_t xbt_graph_new_edge(xbt_graph_t g, xbt_node_t src, xbt_node_t dst, 
			      void *data);
void xbt_graph_edge_set_length(xbt_edge_t e, double length);
double xbt_graph_edge_get_length(xbt_edge_t e);
double* xbt_graph_get_length_matrix(xbt_graph_t g);

void xbt_graph_free_node(xbt_graph_t g, xbt_node_t n, 
			   void_f_pvoid_t *node_free_function , void_f_pvoid_t *edge_free_function);
void xbt_graph_free_edge(xbt_graph_t g, xbt_edge_t e, 
			   void_f_pvoid_t *free_function);
void xbt_graph_free_graph(xbt_graph_t g, 
			  void_f_pvoid_t *node_free_function,
			  void_f_pvoid_t *edge_free_function,
			  void_f_pvoid_t *graph_free_function);

int __xbt_find_in_dynar(xbt_dynar_t dynar, void *p);

xbt_dynar_t xbt_graph_get_nodes(xbt_graph_t g);
xbt_dynar_t xbt_graph_get_edges(xbt_graph_t g);
xbt_node_t xbt_graph_edge_get_source(xbt_edge_t e);
xbt_node_t xbt_graph_edge_get_target(xbt_edge_t e);
xbt_graph_t xbt_graph_read(const char *filename);

/* Not implemented yet ! */
void xbt_export_graphviz(xbt_graph_t g, const char *filename,
			 const char *(node_name)(xbt_node_t),
			 const char *(edge_name)(xbt_edge_t)
			 );

void xbt_graph_export_surfxml(xbt_graph_t g,
			      const char *filename,
			      const char *(node_name)(xbt_node_t),
			      const char *(edge_name)(xbt_edge_t)
			      );

/* void *xbt_graph_to_array(xbt_graph_t g);  */
xbt_node_t* xbt_graph_shortest_paths(xbt_graph_t g);
void xbt_graph_topological_sort(xbt_graph_t g);




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

#endif				/* _XBT_GRAPH_H */






