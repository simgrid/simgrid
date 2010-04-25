/* Copyright (c) 2006, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPH_PRIVATE_H
#define _XBT_GRAPH_PRIVATE_H
#include "xbt/dynar.h"

#define NOT_EXPLORED 0
#define CURRENTLY_EXPLORING 1
#define ALREADY_EXPLORED 2

/* Node structure */
/* typedef struct xbt_node *xbt_node_t; */
typedef struct xbt_node {
  xbt_dynar_t out;
  xbt_dynar_t in;               /* not used when the graph is directed */
  double position_x;            /* positive value: negative means undefined */
  double position_y;            /* positive value: negative means undefined */
  void *data;                   /* user data */
  void *xbtdata;                /* private xbt data: should be reinitialized at the
                                   beginning of your algorithm if you need to use it */
} s_xbt_node_t;

/* edge structure */
/* typedef struct xbt_edge *xbt_edge_t; */
typedef struct xbt_edge {
  xbt_node_t src;
  xbt_node_t dst;
  void *data;                   /* user data */
  void *xbtdata;                /* private xbt data: should be reinitialized at the
                                   beginning of your algorithm if you need to use it */
  double length;                /* positive value: negative means undefined */
} s_xbt_edge_t;

/* Graph structure */
/* typedef struct xbt_graph *xbt_graph_t; */
typedef struct xbt_graph {
  xbt_dynar_t nodes;
  xbt_dynar_t edges;
  unsigned short int directed;
  void *data;                   /* user data */
  void *xbtdata;                /* private xbt data: should be reinitialized at the
                                   beginning of your algorithm if you need to use it */
} s_xbt_graph_t;
void xbt_floyd_algorithm(xbt_graph_t g, double *adj, double *d,
                         xbt_node_t * p);
void xbt_graph_depth_visit(xbt_graph_t g, xbt_node_t n, xbt_node_t * sorted,
                           int *idx);

#endif /* _XBT_GRAPH_PRIVATE_H */
