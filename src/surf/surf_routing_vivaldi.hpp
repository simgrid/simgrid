#include "surf_routing_generic.hpp"
#include "network.hpp"

#ifndef SURF_ROUTING_VIVALDI_HPP_
#define SURF_ROUTING_VIVALDI_HPP_

/***********
 * Classes *
 ***********/
class AsVivaldi;
typedef AsVivaldi *AsVivaldiPtr;

class AsVivaldi: public AsGeneric {
public:
  sg_platf_route_cbarg_t *p_routingTable;

  AsVivaldi() : AsGeneric() {};
  ~AsVivaldi() {};

  void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
  //void getGraph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
  //sg_platf_route_cbarg_t getBypassRoute(RoutingEdgePtr src, RoutingEdgePtr dst, double *lat);

  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int parsePU(RoutingEdgePtr elm); /* A host or a router, whatever */
  //virtual int parseAS( RoutingEdgePtr elm)=0;

  //virtual void parseBypassroute(sg_platf_route_cbarg_t e_route)=0;
};


#endif /* SURF_ROUTING_VIVALDI_HPP_ */
