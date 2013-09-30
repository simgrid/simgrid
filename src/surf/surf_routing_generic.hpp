#include "surf_routing_none.hpp"

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

class AsGeneric;
typedef AsGeneric *AsGenericPtr;

class AsGeneric : public AsNone {
public:
  AsGeneric();
  ~AsGeneric();
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

  xbt_dynar_t getOnelinkRoutes();
  sg_platf_route_cbarg_t getBypassroute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);
  sg_platf_route_cbarg_t newExtendedRoute(e_surf_routing_hierarchy_t hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
  AsPtr asExist(AsPtr to_find);
  AsPtr autonomousSystemExist(char *element);
  AsPtr processingUnitsExist(char *element);
  void srcDstCheck(RoutingEdgePtr src, RoutingEdgePtr dst);
};

#endif /* SURF_ROUTING_GENERIC_HPP_ */
