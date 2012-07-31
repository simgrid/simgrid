#ifndef SG_PLATF_GEN_PRIVATE_H
#define SG_PLATF_GEN_PRIVATE_H

#include "xbt/graph.h"
#include "simgrid/platf.h"

typedef struct s_context_node_t {
  unsigned long id;
  double x, y;
  int degree;
  e_platf_node_kind kind;
  union {
    s_sg_platf_host_cbarg_t host_parameters;
    s_sg_platf_cluster_cbarg_t cluster_parameters;
  };
} s_context_node_t, *context_node_t;

typedef struct s_context_edge_t {
  unsigned long id;
  s_sg_platf_link_cbarg_t link_parameters;
} s_context_edge_t, *context_edge_t;

void platf_graph_init(unsigned long node_count);

void platf_node_connect(xbt_node_t node1, xbt_node_t node2);

double platf_node_distance(xbt_node_t node1, xbt_node_t node2);

double random_pareto(double min, double max, double K, double P, double ALPHA);

#endif      /* SG_PLATF_GEN_PRIVATE_H */
