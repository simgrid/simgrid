

#include "simgrid/platf_generator.h"
#include "platf_generator_private.h"
#include "xbt.h"
#include "xbt/RngStream.h"
#include <math.h>

static xbt_graph_t platform_graph = NULL;

static RngStream rng_stream = NULL;

static unsigned long last_link_id = 0;

xbt_graph_t platf_graph_get(void) {
  // We need some debug, so let's add this function
  // WARNING : shold be removed when it becomes useless
  return platform_graph;
}

void platf_random_seed(unsigned long seed[6]) {

  if(rng_stream == NULL) {
    //stream not created yet, we do it now
    rng_stream = RngStream_CreateStream(NULL);
  }
  if(seed != NULL) {
    RngStream_SetSeed(rng_stream, seed);
  }
}

void platf_graph_init(unsigned long node_count) {
  unsigned long i;
  platform_graph = xbt_graph_new_graph(FALSE, NULL);
  if(rng_stream == NULL) {
    rng_stream = RngStream_CreateStream(NULL);
  }

  for(i=0 ; i<node_count ; i++) {
    context_node_t node_data = NULL;
    node_data = xbt_new(s_context_node_t, 1);
    node_data->id = i+1;
    node_data->x = 0;
    node_data->y = 0;
    node_data->degree = 0;
    node_data->kind = ROUTER;
    xbt_graph_new_node(platform_graph, (void*) node_data);
  }

  last_link_id = 0;

}

void platf_node_connect(xbt_node_t node1, xbt_node_t node2) {
  context_node_t node1_data;
  context_node_t node2_data;
  node1_data = (context_node_t) xbt_graph_node_get_data(node1);
  node2_data = (context_node_t) xbt_graph_node_get_data(node2);
  node1_data->degree++;
  node2_data->degree++;

  unsigned long *link_id = xbt_new(unsigned long, 1);
  *link_id = ++last_link_id;

  xbt_graph_new_edge(platform_graph, node1, node2, (void*)link_id);
}

void platf_graph_uniform(unsigned long node_count) {
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

void platf_graph_heavytailed(unsigned long node_count) {
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t graph_node = NULL;
  context_node_t node_data = NULL;
  unsigned int i;
  platf_graph_init(node_count);
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    node_data = (context_node_t) xbt_graph_node_get_data(graph_node);
    node_data->x = random_pareto(0, 1, 1.0/*K*/, 10e9/*P*/, 1.0/*alpha*/);
    node_data->y = random_pareto(0, 1, 1.0/*K*/, 10e9/*P*/, 1.0/*alpha*/);
  }
}

void platf_graph_interconnect_star(void) {
  /* All the nodes are connected to the first one */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t graph_node = NULL;
  xbt_node_t first_node = NULL;
  unsigned int i;

  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    if(i==0) {
      //Ok, we get the first node, let's keep it somewhere...
      first_node = graph_node;
    } else {
      //All the other nodes are connected to the first one
      platf_node_connect(graph_node, first_node);
    }
  }
}

void platf_graph_interconnect_line(void) {
  /* Node are connected to the previous and the next node, in a line */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t graph_node = NULL;
  xbt_node_t old_node = NULL;
  unsigned int i;

  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    if(old_node != NULL) {
      platf_node_connect(graph_node, old_node);
    }
    old_node = graph_node;
  }
}

void platf_graph_interconnect_ring(void) {
  /* Create a simple topology where all nodes are connected along a ring */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t graph_node = NULL;
  xbt_node_t old_node = NULL;
  xbt_node_t first_node = NULL;
  unsigned int i;

  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    if(i == 0) {
      // this is the first node, let's keep it somewhere
      first_node = graph_node;
    } else {
      //connect each node to the previous one
      platf_node_connect(graph_node, old_node);
    }
    old_node = graph_node;
  }
  //we still have to connect the first and the last node together
  platf_node_connect(first_node, graph_node);
}

void platf_graph_interconnect_clique(void) {
  /* Create a simple topology where all nodes are connected to each other, in a clique manner */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  unsigned int i,j;

  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      platf_node_connect(first_node, second_node);
    }
  }
}



/* Functions used to generate interesting random values */

double random_pareto(double min, double max, double K, double P, double ALPHA) {
  double x = RngStream_RandU01(rng_stream);
  double den = pow(1.0 - x + x*pow(K/P, ALPHA), 1.0/ALPHA);
  double res = (1/den);
  res += min - 1; // pareto is on [1, infinity) by default
  if (res>max) {
    return max;
  }
  return res;
}
