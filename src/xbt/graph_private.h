/* 	$Id$	 */

/* Copyright (c) 2006 Darina Dimitrova, Arnaud Legrand. 
   All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPH_PRIVATE_H
#define _XBT_GRAPH_PRIVATE_H
#include "xbt/dynar.h"

/* Node structure */
/* typedef struct xbt_node *xbt_node_t; */
typedef struct xbt_node 
{
  xbt_dynar_t out;
  xbt_dynar_t in;  /* not used when the graph is directed */
  void *data;      /* used data */
  void *xbtdata;   /* private xbt data */  
} s_xbt_node_t;

/* edge structure */
/* typedef struct xbt_edge *xbt_edge_t; */
typedef struct xbt_edge 
{
  xbt_node_t src;
  xbt_node_t dst;
  void *data;      /* used data */
  void *xbtdata;   /* private xbt data */
  double length;
} s_xbt_edge_t;

/* Graph structure */
/* typedef struct xbt_graph *xbt_graph_t; */
typedef struct xbt_graph 
{
  xbt_dynar_t nodes;
  xbt_dynar_t edges;
  unsigned short int directed;
  void *data;      /* used data */
  void *xbtdata;   /* private xbt data */  
} s_xbt_graph_t;
void xbt_floyd_algorithm(xbt_graph_t g, double* adj,double* d,  xbt_node_t* p);


#endif				/* _XBT_GRAPH_PRIVATE_H */
