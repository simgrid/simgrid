

#include "simgrid/platf_generator.h"
#include "platf_generator_private.h"
#include "xbt.h"
#include "xbt/RngStream.h"
#include <math.h>

static xbt_graph_t platform_graph = NULL;
static xbt_dynar_t promoter_dynar = NULL;
static xbt_dynar_t labeler_dynar = NULL;

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
    node_data = xbt_new0(s_context_node_t, 1);
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

  context_edge_t edge_data = NULL;
  edge_data = xbt_new0(s_context_edge_t, 1);
  edge_data->id = ++last_link_id;
  edge_data->labeled = FALSE;
  xbt_graph_new_edge(platform_graph, node1, node2, (void*)edge_data);
}

double platf_node_distance(xbt_node_t node1, xbt_node_t node2) {
  context_node_t node1_data;
  context_node_t node2_data;
  double delta_x;
  double delta_y;
  double distance;
  node1_data = (context_node_t) xbt_graph_node_get_data(node1);
  node2_data = (context_node_t) xbt_graph_node_get_data(node2);
  delta_x = node1_data->x - node2_data->x;
  delta_y = node1_data->y - node2_data->y;
  distance = sqrt(delta_x*delta_x + delta_y*delta_y);
  return distance;
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

void platf_graph_interconnect_uniform(double alpha) {
  /* Creates a topology where the probability to connect two nodes is uniform (unrealistic, but simple)
     alpha : Probability for two nodes to get connected */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  unsigned int i,j;

  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      if(j>=i)
        break;
      if(RngStream_RandU01(rng_stream) < alpha) {
        platf_node_connect(first_node, second_node);
      }
    }
  }
}

void platf_graph_interconnect_exponential(double alpha) {
  /* Create a topology where the probability follows an exponential law
     Number of edges increases with alpha */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  unsigned int i,j;
  double L = sqrt(2.0); /*  L = c*sqrt(2); c=side of placement square */
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      if(j>=i)
        break;
      double d = platf_node_distance(first_node, second_node);
      if(RngStream_RandU01(rng_stream) < alpha*exp(-d/(L-d))) {
        platf_node_connect(first_node, second_node);
      }
    }
  }
}

void platf_graph_interconnect_waxman(double alpha, double beta) {
  /* Create a topology where the probability follows the model of Waxman
   * (see Waxman, Routing of Multipoint Connections, IEEE J. on Selected Areas in Comm., 1988)
   *
   * Number of edges increases with alpha
   * edge length heterogeneity increases with beta
   */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  unsigned int i,j;
  double L = sqrt(2.0); /*  L = c*sqrt(2); c=side of placement square */
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      if(j>=i)
        break;
      double d = platf_node_distance(first_node, second_node);
      if(RngStream_RandU01(rng_stream) < alpha*exp(-d/(L*beta))) {
        platf_node_connect(first_node, second_node);
      }
    }
  }
}

void platf_graph_interconnect_zegura(double alpha, double beta, double r) {
  /* Create a topology where the probability follows the model of Zegura
   * (see Zegura, Calvert, Donahoo, A quantitative comparison of graph-based models
   * for Internet topology, IEEE/ACM Transactions on Networking, 1997.)
   *
   * alpha : Probability of connexion for short edges
   * beta : Probability of connexion for long edges
   * r : Limit between long and short edges (between 0 and sqrt(2) since nodes are placed on the unit square)
   */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  unsigned int i,j;
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      if(j>=i)
        break;
      double d = platf_node_distance(first_node, second_node);
      double proba = d < r ? alpha : beta;
      if(RngStream_RandU01(rng_stream) < proba) {
        platf_node_connect(first_node, second_node);
      }
    }
  }
}

void platf_graph_interconnect_barabasi(void) {
  /* Create a topology constructed according to the Barabasi-Albert algorithm (follows power laws)
     (see Barabasi and Albert, Emergence of scaling in random networks, Science 1999, num 59, p509­-512.) */
  xbt_dynar_t dynar_nodes = NULL;
  xbt_node_t first_node = NULL;
  xbt_node_t second_node = NULL;
  context_node_t node_data = NULL;
  unsigned int i,j;
  unsigned long sum = 0;
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, first_node) {
    xbt_dynar_foreach(dynar_nodes, j, second_node) {
      if(j>=i)
        break;
      node_data = xbt_graph_node_get_data(second_node);
      if(sum==0 || RngStream_RandU01(rng_stream) < ((double)(node_data->degree)/ (double)sum)) {
        platf_node_connect(first_node, second_node);
        sum += 2;
      }
    }
  }
}

void platf_graph_promote_to_host(context_node_t node, sg_platf_host_cbarg_t parameters) {
  node->kind = HOST;
  memcpy(&(node->host_parameters), parameters, sizeof(s_sg_platf_host_cbarg_t));
}

void platf_graph_promote_to_cluster(context_node_t node, sg_platf_cluster_cbarg_t parameters) {
  node->kind = CLUSTER;
  memcpy(&(node->cluster_parameters), parameters, sizeof(s_sg_platf_cluster_cbarg_t));
}

void platf_graph_link_label(context_edge_t edge, sg_platf_link_cbarg_t parameters) {
  memcpy(&(edge->link_parameters), parameters, sizeof(s_sg_platf_link_cbarg_t));
}

void platf_graph_promoter(platf_promoter_cb_t promoter_callback) {
  if(promoter_dynar == NULL) {
    promoter_dynar = xbt_dynar_new(sizeof(platf_promoter_cb_t), NULL);
  }
  xbt_dynar_push(promoter_dynar, &promoter_callback);
}

void platf_graph_labeler(platf_labeler_cb_t labeler_callback) {
  if(labeler_dynar == NULL) {
    labeler_dynar = xbt_dynar_new(sizeof(void*), NULL);
  }
  xbt_dynar_push(labeler_dynar, &labeler_callback);
}

void platf_do_promote(void) {
  platf_promoter_cb_t promoter_callback;
  xbt_node_t graph_node = NULL;
  xbt_dynar_t dynar_nodes = NULL;
  context_node_t node = NULL;
  unsigned int i, j;
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    node = (context_node_t) xbt_graph_node_get_data(graph_node);
    xbt_dynar_foreach(promoter_dynar, j, promoter_callback) {
      if(node->kind != ROUTER)
        break;
      promoter_callback(node);
    }
  }
}

void platf_do_label(void) {
  platf_labeler_cb_t labeler_callback;
  xbt_edge_t graph_edge = NULL;
  xbt_dynar_t dynar_edges = NULL;
  context_edge_t edge = NULL;
  unsigned int i, j;
  dynar_edges = xbt_graph_get_edges(platform_graph);
  xbt_dynar_foreach(dynar_edges, i, graph_edge) {
    edge = (context_edge_t) xbt_graph_edge_get_data(graph_edge);
    xbt_dynar_foreach(promoter_dynar, j, labeler_callback) {
      if(edge->labeled == TRUE)
        break;
      labeler_callback(edge);
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
