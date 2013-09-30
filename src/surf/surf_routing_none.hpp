#include "surf_routing.hpp"

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

class AsNone : public As {
public:
  AsNone();
  ~AsNone();
  void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  xbt_dynar_t getOneLinkRoutes();
  void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);
  void finalize();

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int parsePU(RoutingEdgePtr elm); /* A host or a router, whatever */
  int parseAS( RoutingEdgePtr elm);
  void parseRoute(sg_platf_route_cbarg_t route);
  void parseASroute(sg_platf_route_cbarg_t route);
  void parseBypassroute(sg_platf_route_cbarg_t e_route);
};


#endif /* SURF_ROUTING_NONE_HPP_ */
