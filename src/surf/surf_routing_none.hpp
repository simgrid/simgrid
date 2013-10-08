#include "surf_routing.hpp"

#ifndef SURF_ROUTING_NONE_HPP_
#define SURF_ROUTING_NONE_HPP_

class AsNone : public As {
public:
  AsNone();
  ~AsNone();
  virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  virtual xbt_dynar_t getOneLinkRoutes();
  virtual void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  virtual sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  virtual int parsePU(RoutingEdgePtr elm); /* A host or a router, whatever */
  virtual int parseAS( RoutingEdgePtr elm);
  virtual void parseRoute(sg_platf_route_cbarg_t route);
  virtual void parseASroute(sg_platf_route_cbarg_t route);
  virtual void parseBypassroute(sg_platf_route_cbarg_t e_route);
};


#endif /* SURF_ROUTING_NONE_HPP_ */
