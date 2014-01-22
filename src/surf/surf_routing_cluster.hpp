#include "surf_routing_none.hpp"
#include "network_interface.hpp"

#ifndef SURF_ROUTING_CLUSTER_HPP_
#define SURF_ROUTING_CLUSTER_HPP_

/***********
 * Classes *
 ***********/
class AsCluster;
typedef AsCluster *AsClusterPtr;


/* ************************************************** */
/* **************  Cluster ROUTING   **************** */

class AsCluster: public AsNone {
public:
  AsCluster();

  virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  //xbt_dynar_t getOneLinkRoutes();
  //void parseRoute(sg_platf_route_cbarg_t route);
  //void parseASroute(sg_platf_route_cbarg_t route);

  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  //sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int parsePU(RoutingEdgePtr elm); /* A host or a router, whatever */
  int parseAS(RoutingEdgePtr elm);
  virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
  NetworkLinkPtr p_backbone;
  void *p_loopback;
  RoutingEdgePtr p_router;
  int p_has_limiter;
  int p_has_loopback;
  int p_nb_links_per_node;

};


#endif /* SURF_ROUTING_CLUSTER_HPP_ */
