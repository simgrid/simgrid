#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/dynar.h"

/* Node structure */
typedef struct xbt_node *xbt_node_t;
typedef struct xbt_node {
  xbt_dynar_t out;
  xbt_dynar_t in;
  xbt_node_t *route;
  void *data;
} s_xbt_node_t;

/* edge structure */
typedef struct xbt_edge *xbt_edge_t;
typedef struct xbt_edge {
  xbt_node_t src;
  xbt_node_t dst;
  void *data;
} s_xbt_edge_t;

/* Graph structure */
typedef struct xbt_graph *xbt_graph_t;
typedef struct xbt_graph {
  char *name;
  xbt_dynar_t nodes;
  xbt_dynar_t edges;
  unsigned short int directed;
  void *data;
} s_xbt_graph_t;

