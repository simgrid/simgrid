

#include <simgrid/platf_generator.h>
#include "platf_generator_private.h"
#include <xbt.h>
#include <xbt/graph.h>
#include <xbt/RngStream.h>

static xbt_graph_t platform_graph = NULL;

static RngStream rng_stream = NULL;

void platf_random_seed(unsigned long seed[6]) {

  if(rng_stream == NULL) {
    //stream not created yet, we do it now
    rng_stream = RngStream_CreateStream(NULL);
  }
  if(seed != NULL) {
    RngStream_SetSeed(rng_stream, seed);
  }
}

void platf_graph_init(int node_count) {
  int i;
  platform_graph = xbt_graph_new_graph(TRUE, NULL);
  if(rng_stream == NULL) {
    rng_stream = RngStream_CreateStream(NULL);
  }

  for(i=0 ; i<node_count ; i++) {
    context_node_t node_data = NULL;
    node_data = xbt_new(s_context_node_t, 1);
    node_data->x = 0;
    node_data->y = 0;
    node_data->degree = 0;
    node_data->kind = ROUTER;
    xbt_graph_new_node(platform_graph, (void*) node_data);
  }
}

void platf_graph_uniform(int node_count) {
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t graph_node = NULL;
  context_node_t node_data = NULL;
  unsigned int i;
  platf_graph_init(node_count);
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    node_data = (context_node_t) xbt_graph_node_get_data(graph_node);
    node_data->x = RngStream_RandU01(rng_stream);
    node_data->y = RngStream_RandU01(rng_stream);
  }
}
