
/* platf_generator.h - Public interface to the SimGrid platforms generator  */

/* Copyright (c) 2004-2012. The SimGrid Team.
 * All rights reserved.                                                     */

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

typedef struct s_context_node_t {
  unsigned long id;
  double x, y;
  int degree;
  e_platf_node_kind kind;
  int connect_checked;
  union {
    s_sg_platf_host_cbarg_t host_parameters;
    s_sg_platf_cluster_cbarg_t cluster_parameters;
    char* router_id;
  };
} s_context_node_t, *context_node_t;

typedef struct s_context_edge_t {
  unsigned long id;
  double length;
  int labeled;
  s_sg_platf_link_cbarg_t link_parameters;
} s_context_edge_t, *context_edge_t;

typedef void (*platf_promoter_cb_t) (context_node_t);
typedef void (*platf_labeler_cb_t) (context_edge_t);

XBT_PUBLIC(void) platf_random_seed(unsigned long seed[6]);

XBT_PUBLIC(void) platf_graph_uniform(unsigned long node_count);
XBT_PUBLIC(void) platf_graph_heavytailed(unsigned long node_count);

XBT_PUBLIC(void) platf_graph_interconnect_star(void);
XBT_PUBLIC(void) platf_graph_interconnect_line(void);
XBT_PUBLIC(void) platf_graph_interconnect_ring(void);
XBT_PUBLIC(void) platf_graph_interconnect_clique(void);
XBT_PUBLIC(void) platf_graph_interconnect_uniform(double alpha);
XBT_PUBLIC(void) platf_graph_interconnect_exponential(double alpha);
XBT_PUBLIC(void) platf_graph_interconnect_zegura(double alpha, double beta, double r);
XBT_PUBLIC(void) platf_graph_interconnect_waxman(double alpha, double beta);
XBT_PUBLIC(void) platf_graph_interconnect_barabasi(void);

XBT_PUBLIC(int) platf_graph_is_connected(void);

XBT_PUBLIC(void) platf_graph_clear_links(void);

XBT_PUBLIC(void) platf_graph_promote_to_host(context_node_t node, sg_platf_host_cbarg_t parameters);
XBT_PUBLIC(void) platf_graph_promote_to_cluster(context_node_t node, sg_platf_cluster_cbarg_t parameters);

XBT_PUBLIC(void) platf_graph_link_label(context_edge_t edge, sg_platf_link_cbarg_t parameters);

XBT_PUBLIC(void) platf_graph_promoter(platf_promoter_cb_t promoter_callback);
XBT_PUBLIC(void) platf_graph_labeler(platf_labeler_cb_t labeler_callback);

XBT_PUBLIC(void) platf_do_promote(void);
XBT_PUBLIC(void) platf_do_label(void);

XBT_PUBLIC(void) platf_generate(void);

// WARNING : Only for debbugging ; should be removed when platform
// generation works correctly
XBT_PUBLIC(xbt_graph_t) platf_graph_get(void);

#endif              /* SG_PLATF_GEN_H */

