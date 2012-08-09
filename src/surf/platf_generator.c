

#include "simgrid/platf_generator.h"
#include "platf_generator_private.h"
#include "xbt.h"
#include "xbt/RngStream.h"
#include "surf/simgrid_dtd.h"
#include "surf_private.h"
#include <math.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(platf_generator, surf, "Platform Generator");

static xbt_graph_t platform_graph = NULL;
static xbt_dynar_t promoter_dynar = NULL;
static xbt_dynar_t labeler_dynar = NULL;

static RngStream rng_stream = NULL;

static unsigned long last_link_id = 0;

xbt_graph_t platf_graph_get(void) {
  // We need some debug, so let's add this function
  // WARNING : should be removed when it becomes useless
  return platform_graph;
}

/**
 * \brief Set the seed of the platform generator RngStream
 *
 * This RngStream is used to generate all the random values needed to
 * generate the platform
 *
 * \param seed A array of six integer; if NULL, the default seed will be used.
 */
void platf_random_seed(unsigned long seed[6]) {

  if(rng_stream == NULL) {
    //stream not created yet, we do it now
    rng_stream = RngStream_CreateStream(NULL);
  }
  if(seed != NULL) {
    RngStream_SetSeed(rng_stream, seed);
  }
}

/**
 * \brief Initialize the platform generator
 *
 * This function create the graph and add node_count nodes to it
 * \param node_count The number of nodes of the platform
 */
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

/**
 * \brief Connect two nodes
 * \param node1 The first node to connect
 * \param node2 The second node to connect
 */
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
  edge_data->length = platf_node_distance(node1, node2);
  edge_data->labeled = FALSE;
  xbt_graph_new_edge(platform_graph, node1, node2, (void*)edge_data);
}

/**
 * \brief Compute the distance between two nodes
 * \param node1 The first node
 * \param node2 The second node
 * \return The distance between node1 and node2
 */
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

/**
 * \brief Initialize the platform, placing nodes uniformly on the unit square
 * \param node_count The number of node
 */
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

/**
 * \brief Initialize the platform, placing nodes in little clusters on the unit square
 * \param node_count The number of node
 */
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

/**
 * \brief Creates a simple topology where all nodes are connected to the first one in a star fashion
 */
void platf_graph_interconnect_star(void) {
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

/**
 * \brief Creates a simple topology where all nodes are connected in line
 */
void platf_graph_interconnect_line(void) {
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

/**
 * \brief Create a simple topology where all nodes are connected along a ring
 */
void platf_graph_interconnect_ring(void) {
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

/**
 * \brief Create a simple topology where all nodes are connected to each other, in a clique manner
 */
void platf_graph_interconnect_clique(void) {
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

/**
 * \brief Creates a topology where the probability to connect two nodes is uniform (unrealistic, but simple)
 * \param alpha Probability for two nodes to get connected
 */
void platf_graph_interconnect_uniform(double alpha) {
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

/**
 * \brief Create a topology where the probability follows an exponential law
 * \param alpha Number of edges increases with alpha
 */
void platf_graph_interconnect_exponential(double alpha) {
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

/**
 * \brief Create a topology where the probability follows the model of Waxman
 *
 * see Waxman, Routing of Multipoint Connections, IEEE J. on Selected Areas in Comm., 1988
 *
 * \param alpha Number of edges increases with alpha
 * \param beta Edge length heterogeneity increases with beta
 */
void platf_graph_interconnect_waxman(double alpha, double beta) {
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

/**
 * \brief Create a topology where the probability follows the model of Zegura
 * see Zegura, Calvert, Donahoo, A quantitative comparison of graph-based models
 * for Internet topology, IEEE/ACM Transactions on Networking, 1997.
 *
 * \param alpha Probability of connexion for short edges
 * \param beta Probability of connexion for long edges
 * \param r Limit between long and short edges (between 0 and sqrt(2) since nodes are placed on the unit square)
 */
void platf_graph_interconnect_zegura(double alpha, double beta, double r) {
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

/**
 * \brief Create a topology constructed according to the Barabasi-Albert algorithm (follows power laws)
 * see Barabasi and Albert, Emergence of scaling in random networks, Science 1999, num 59, p509­-512.
 */
void platf_graph_interconnect_barabasi(void) {
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

/**
 * \brief Check if the produced graph is connected
 *
 * You should check if the produced graph is connected before doing anything
 * on it. You probably don't want any isolated node or group of nodes...
 *
 * \return TRUE if the graph is connected, FALSE otherwise
 */
int platf_graph_is_connected(void) {
  xbt_dynar_t dynar_nodes = NULL;
  xbt_dynar_t connected_nodes = NULL;
  xbt_dynar_t outgoing_edges = NULL;
  xbt_node_t graph_node = NULL;
  xbt_edge_t outedge = NULL;
  unsigned long iterator;
  unsigned int i;
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  connected_nodes = xbt_dynar_new(sizeof(xbt_node_t), NULL);

  //Initialize the connected node array with the first node
  xbt_dynar_get_cpy(dynar_nodes, 0, &graph_node);
  xbt_dynar_push(connected_nodes, &graph_node);
  iterator = 0;
  do {
    //Get the next node
    xbt_dynar_get_cpy(connected_nodes, iterator, &graph_node);

    //add all the linked nodes to the connected node array
    outgoing_edges = xbt_graph_node_get_outedges(graph_node);
    xbt_dynar_foreach(outgoing_edges, i, outedge) {
      xbt_node_t src = xbt_graph_edge_get_source(outedge);
      xbt_node_t dst = xbt_graph_edge_get_target(outedge);
      if(!xbt_dynar_member(connected_nodes, &src)) {
        xbt_dynar_push(connected_nodes, &src);
      }
      if(!xbt_dynar_member(connected_nodes, &dst)) {
        xbt_dynar_push(connected_nodes, &dst);
      }
    }
  } while(++iterator < xbt_dynar_length(connected_nodes));

  // The graph is connected if the connected node array has the same length
  // as the graph node array
  return xbt_dynar_length(connected_nodes) == xbt_dynar_length(dynar_nodes);
}


/**
 * \brief Remove the links in the created topology
 *
 * This is useful when the created topology is not connected, and you want
 * to generate a new one.
 */
void platf_graph_clear_links(void) {
  xbt_dynar_t dynar_nodes = NULL;
  xbt_dynar_t dynar_edges = NULL;
  xbt_node_t graph_node = NULL;
  xbt_edge_t graph_edge = NULL;
  context_node_t node_data = NULL;
  unsigned int i;

  //Delete edges from the graph
  dynar_edges = xbt_graph_get_edges(platform_graph);
  xbt_dynar_foreach(dynar_edges, i, graph_edge) {
    xbt_graph_free_edge(platform_graph, graph_edge, xbt_free);
  }

  //All the nodes will be of degree 0
  dynar_nodes = xbt_graph_get_nodes(platform_graph);
  xbt_dynar_foreach(dynar_nodes, i, graph_node) {
    node_data = xbt_graph_node_get_data(graph_node);
    node_data->degree = 0;
  }
}

/**
 * \brief Promote a node to a host
 *
 * This function should be called in callbacks registered with the
 * platf_graph_promoter function.
 *
 * \param node The node to promote
 * \param parameters The parameters needed to build the host
 */
void platf_graph_promote_to_host(context_node_t node, sg_platf_host_cbarg_t parameters) {
  node->kind = HOST;
  memcpy(&(node->host_parameters), parameters, sizeof(s_sg_platf_host_cbarg_t));
}

/**
 * \brief Promote a node to a cluster
 *
 * This function should be called in callbacks registered with the
 * platf_graph_promoter function.
 *
 * \param node The node to promote
 * \param parameters The parameters needed to build the cluster
 */
void platf_graph_promote_to_cluster(context_node_t node, sg_platf_cluster_cbarg_t parameters) {
  node->kind = CLUSTER;
  memcpy(&(node->cluster_parameters), parameters, sizeof(s_sg_platf_cluster_cbarg_t));
}

/**
 * \brief Set the parameters of a network link.
 *
 * This function should be called in callbacks registered with the
 * platf_graph_labeler function.
 *
 * \param edge The edge to modify
 * \param parameters The parameters of the network link
 */
void platf_graph_link_label(context_edge_t edge, sg_platf_link_cbarg_t parameters) {
  memcpy(&(edge->link_parameters), parameters, sizeof(s_sg_platf_link_cbarg_t));
  edge->labeled = TRUE;
}

/**
 * \brief Register a callback to promote nodes
 *
 * The best way to promote nodes into host or cluster is to write a function
 * which takes one parameter, a #context_node_t, make every needed test on
 * it, and call platf_graph_promote_to_host or platf_graph_promote_to_cluster
 * if needed. Then, register the function with this one.
 * You can register several callbacks: the first registered function will be
 * called first. If the node have not been promoted yet, the second function
 * will be called, and so on...
 *
 * \param promoter_callback The callback function
 */
void platf_graph_promoter(platf_promoter_cb_t promoter_callback) {
  if(promoter_dynar == NULL) {
    promoter_dynar = xbt_dynar_new(sizeof(platf_promoter_cb_t), NULL);
  }
  xbt_dynar_push(promoter_dynar, &promoter_callback);
}

/**
 * \brief Register a callback to label links
 *
 * Like the node promotion, it is better, to set links, to write a function
 * which take one parameter, a #context_edge_t, make every needed test on
 * it, and call platf_graph_link_label if needed.
 * You can register several callbacks: the first registered function will be
 * called first. If the link have not been labeled yet, the second function
 * will be called, and so on... All the links must have been labeled after
 * all the calls.
 *
 * \param labeler_callback The callback function
 */
void platf_graph_labeler(platf_labeler_cb_t labeler_callback) {
  if(labeler_dynar == NULL) {
    labeler_dynar = xbt_dynar_new(sizeof(void*), NULL);
  }
  xbt_dynar_push(labeler_dynar, &labeler_callback);
}

/**
 * \brief Call the registered promoters on all nodes
 *
 * The promoters are called on all nodes, in the order of their registration
 * If some nodes are not promoted, they will be routers
 */
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

/**
 * \brief Call the registered labelers on all links
 */
void platf_do_label(void) {
  platf_labeler_cb_t labeler_callback;
  xbt_edge_t graph_edge = NULL;
  xbt_dynar_t dynar_edges = NULL;
  context_edge_t edge = NULL;
  unsigned int i, j;
  dynar_edges = xbt_graph_get_edges(platform_graph);
  xbt_dynar_foreach(dynar_edges, i, graph_edge) {
    edge = (context_edge_t) xbt_graph_edge_get_data(graph_edge);
    xbt_dynar_foreach(labeler_dynar, j, labeler_callback) {
      if(edge->labeled)
        break;
      labeler_callback(edge);
    }
    if(!edge->labeled) {
      XBT_ERROR("All links of the generated platform are not labeled.");
      xbt_die("Please check your generation parameters.");
    }
  }
}

/**
 * \brief putting into SURF the generated platform
 *
 * This function should be called when the generation is over and the platform
 * is ready to be put in place in SURF. All the init function, like MSG_init,
 * must have been called before, or this function will not do anything.
 * After that function, it should be possible to list all the available hosts
 * with the provided functions.
 */
void platf_generate(void) {

  xbt_dynar_t nodes = NULL;
  xbt_node_t graph_node = NULL;
  context_node_t node_data = NULL;
  xbt_dynar_t edges = NULL;
  xbt_edge_t graph_edge = NULL;
  context_edge_t edge_data = NULL;
  unsigned int i;

  unsigned int last_host = 0;
  unsigned int last_router = 0;
  unsigned int last_cluster = 0;

  sg_platf_host_cbarg_t host_parameters;
  sg_platf_cluster_cbarg_t cluster_parameters;
  sg_platf_link_cbarg_t link_parameters;
  s_sg_platf_router_cbarg_t router_parameters; /* This one is not a pointer! */
  s_sg_platf_route_cbarg_t route_parameters; /* neither this one! */

  router_parameters.coord = NULL;
  route_parameters.symmetrical = FALSE;
  route_parameters.src = NULL;
  route_parameters.dst = NULL;
  route_parameters.gw_dst = NULL;
  route_parameters.gw_src = NULL;
  route_parameters.link_list = NULL;

  nodes = xbt_graph_get_nodes(platform_graph);
  edges = xbt_graph_get_edges(platform_graph);

  sg_platf_begin();
  surf_parse_init_callbacks();
  routing_register_callbacks();


  sg_platf_new_AS_begin("random platform", A_surfxml_AS_routing_Floyd);

  //Generate hosts, clusters and routers
  xbt_dynar_foreach(nodes, i, graph_node) {
    node_data = xbt_graph_node_get_data(graph_node);
    switch(node_data->kind) {
      case HOST:
        host_parameters = &node_data->host_parameters;
        last_host++;
        if(host_parameters->id == NULL) {
          host_parameters->id = bprintf("host-%d", last_host);
        }
        sg_platf_new_host(host_parameters);
        break;
      case CLUSTER:
        cluster_parameters = &node_data->cluster_parameters;
        last_cluster++;
        if(cluster_parameters->prefix == NULL) {
          cluster_parameters->prefix = "host-";
        }
        if(cluster_parameters->suffix == NULL) {
          cluster_parameters->suffix = bprintf(".cluster-%d", last_cluster);
        }
        if(cluster_parameters->id == NULL) {
          cluster_parameters->id = bprintf("cluster-%d", last_cluster);
        }
        sg_platf_new_cluster(cluster_parameters);
        break;
      case ROUTER:
        router_parameters.id = bprintf("router-%d", ++last_router);
        sg_platf_new_router(&router_parameters);
    }
  }

  //Generate links and routes
  xbt_dynar_foreach(edges, i, graph_edge) {
    xbt_node_t src = xbt_graph_edge_get_source(graph_edge);
    xbt_node_t dst = xbt_graph_edge_get_target(graph_edge);
    context_node_t src_data = xbt_graph_node_get_data(src);
    context_node_t dst_data = xbt_graph_node_get_data(dst);
    edge_data = xbt_graph_edge_get_data(graph_edge);
    const char* temp = NULL;

    //Add a link to the platform
    link_parameters = &edge_data->link_parameters;
    if(link_parameters->id == NULL) {
      link_parameters->id = bprintf("link-%ld", edge_data->id);
    }
    sg_platf_new_link(link_parameters);

    //Add a route matching this link
    switch(src_data->kind) {
      case ROUTER:
        route_parameters.src = src_data->router_id;
        break;
      case CLUSTER:
        route_parameters.src = src_data->cluster_parameters.id;
        break;
      case HOST:
        route_parameters.src = src_data->host_parameters.id;
    }
    switch(dst_data->kind) {
      case ROUTER:
        route_parameters.dst = dst_data->router_id;
        break;
      case CLUSTER:
        route_parameters.dst = dst_data->cluster_parameters.id;
        break;
      case HOST:
        route_parameters.dst = dst_data->host_parameters.id;
    }
    sg_platf_route_begin(&route_parameters);
    sg_platf_route_add_link(link_parameters->id, &route_parameters);
    sg_platf_route_end(&route_parameters);

    //Create the symmertical route
    temp = route_parameters.dst;
    route_parameters.dst = route_parameters.src;
    route_parameters.src = temp;
    sg_platf_route_begin(&route_parameters);
    sg_platf_route_add_link(link_parameters->id, &route_parameters);
    sg_platf_route_end(&route_parameters);
  }

  sg_platf_new_AS_end();
  sg_platf_end();
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
