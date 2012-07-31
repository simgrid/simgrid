
/* platf_generator.h - Public interface to the SimGrid platforms generator  */

/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_GEN_H
#define SG_PLATF_GEN_H

#include "xbt.h"
#include "xbt/graph.h" //Only for platf_graph_get()
#include "platf.h"

typedef enum {
  ROUTER,
  HOST,
  CLUSTER
} e_platf_node_kind;

XBT_PUBLIC(void) platf_random_seed(unsigned long seed[6]);

XBT_PUBLIC(void) platf_graph_uniform(unsigned long node_count);
XBT_PUBLIC(void) platf_graph_heavytailed(unsigned long node_count);

XBT_PUBLIC(void) platf_graph_interconnect_star(void);
XBT_PUBLIC(void) platf_graph_interconnect_line(void);
XBT_PUBLIC(void) platf_graph_interconnect_ring(void);
XBT_PUBLIC(void) platf_graph_interconnect_clique(void);
XBT_PUBLIC(void) platf_graph_interconnect_uniform(double alpha);
XBT_PUBLIC(void) platf_graph_interconnect_exponential(double alpha);

XBT_PUBLIC(void) platf_graph_promote_to_host(xbt_node_t node, sg_platf_host_cbarg_t parameters);
XBT_PUBLIC(void) platf_graph_promote_to_cluster(xbt_node_t node, sg_platf_cluster_cbarg_t parameters);

XBT_PUBLIC(void) platf_graph_link_label(xbt_edge_t edge, sg_platf_link_cbarg_t parameters);

// WARNING : Only for debbugging ; should be removed when platform
// generation works correctly
XBT_PUBLIC(xbt_graph_t) platf_graph_get(void);

#endif              /* SG_PLATF_GEN_H */
