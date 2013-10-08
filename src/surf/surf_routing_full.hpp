#include "surf_routing_generic.hpp"

#ifndef SURF_ROUTING_FULL_HPP_
#define SURF_ROUTING_FULL_HPP_

/***********
 * Classes *
 ***********/
class AsFull;
typedef AsFull *AsFullPtr;

class AsFull: public AsGeneric {
public:
  sg_platf_route_cbarg_t *p_routingTable;

  AsFull();
  ~AsFull();
  int test(){return 1;};

  void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  xbt_dynar_t getOneLinkRoutes();
  void parseRoute(sg_platf_route_cbarg_t route);
  void parseASroute(sg_platf_route_cbarg_t route);

  //void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  //sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  //virtual int parsePU(RoutingEdgePtr elm)=0; /* A host or a router, whatever */
  //virtual int parseAS( RoutingEdgePtr elm)=0;

  //virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};


#endif /* SURF_ROUTING_FULL_HPP_ */
