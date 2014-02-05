/* Copyright (c) 2012, 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_GEN_PRIVATE_H
#define SG_PLATF_GEN_PRIVATE_H

#include "xbt/graph.h"
#include "simgrid/platf.h"

void platf_graph_init(unsigned long node_count);

void platf_node_connect(xbt_node_t node1, xbt_node_t node2);

double platf_node_distance(xbt_node_t node1, xbt_node_t node2);

double random_pareto(double min, double max, double K, double P, double ALPHA);

#endif      /* SG_PLATF_GEN_PRIVATE_H */
