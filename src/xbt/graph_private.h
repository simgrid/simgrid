/* Copyright (c) 2006, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPH_PRIVATE_H
#define _XBT_GRAPH_PRIVATE_H
#include "xbt/dynar.h"
#include "xbt/graph.h"

#define NOT_EXPLORED 0
#define CURRENTLY_EXPLORING 1
#define ALREADY_EXPLORED 2

void xbt_floyd_algorithm(xbt_graph_t g, double *adj, double *d,
                         xbt_node_t * p);
void xbt_graph_depth_visit(xbt_graph_t g, xbt_node_t n,
                           xbt_node_t * sorted, int *idx);

#endif                          /* _XBT_GRAPH_PRIVATE_H */
