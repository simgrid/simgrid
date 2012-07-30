#ifndef SG_PLATF_GEN_PRIVATE_H
#define SG_PLATF_GEN_PRIVATE_H

#include <xbt/graph.h>

typedef struct {
  unsigned long id;
  double x, y;
  int degree;
  e_platf_node_kind kind;
} s_context_node_t, *context_node_t;

void platf_graph_init(unsigned long node_count);

void platf_node_connect(xbt_node_t node1, xbt_node_t node2);

double platf_node_distance(xbt_node_t node1, xbt_node_t node2);

double random_pareto(double min, double max, double K, double P, double ALPHA);

#endif      /* SG_PLATF_GEN_PRIVATE_H */
