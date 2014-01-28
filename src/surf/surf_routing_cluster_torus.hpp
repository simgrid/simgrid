#include "surf_routing_none.hpp"
#include "network_interface.hpp"
#include "surf_routing_cluster.hpp"


#ifndef SURF_ROUTING_CLUSTER_TORUS_HPP_
#define SURF_ROUTING_CLUSTER_TORUS_HPP_

class AsClusterTorus;
typedef AsClusterTorus *AsClusterTorusPtr;


class AsClusterTorus: public AsCluster {
public:
   AsClusterTorus();
   virtual ~AsClusterTorus();
   virtual void create_links_for_node(sg_platf_cluster_cbarg_t cluster, int id, int rank, int position);
   virtual void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t into, double *latency);
   void parse_specific_arguments(sg_platf_cluster_cbarg_t cluster);


   xbt_dynar_t p_dimensions;

};


#endif
