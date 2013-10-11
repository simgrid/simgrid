#include "surf_routing_none.hpp"

#ifndef SURF_ROUTING_GENERIC_HPP_
#define SURF_ROUTING_GENERIC_HPP_

class AsGeneric;
typedef AsGeneric *AsGenericPtr;

class AsGeneric : public AsNone {
public:
  AsGeneric();
  ~AsGeneric();

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

  virtual sg_platf_route_cbarg_t newExtendedRoute(e_surf_routing_hierarchy_t hierarchy, sg_platf_route_cbarg_t routearg, int change_order);
  virtual AsPtr asExist(AsPtr to_find);
  virtual AsPtr autonomousSystemExist(char *element);
  virtual AsPtr processingUnitsExist(char *element);
  virtual void srcDstCheck(RoutingEdgePtr src, RoutingEdgePtr dst);
};

#endif /* SURF_ROUTING_GENERIC_HPP_ */
