#include "surf_routing_generic.hpp"

#ifndef SURF_ROUTING_DIJKSTRA_HPP_
#define SURF_ROUTING_DIJKSTRA_HPP_

typedef struct graph_node_data {
  int id;
  int graph_id;                 /* used for caching internal graph id's */
} s_graph_node_data_t, *graph_node_data_t;

typedef struct graph_node_map_element {
  xbt_node_t node;
} s_graph_node_map_element_t, *graph_node_map_element_t;

typedef struct route_cache_element {
  int *pred_arr;
  int size;
} s_route_cache_element_t, *route_cache_element_t;

/***********
 * Classes *
 ***********/
class AsDijkstra;
typedef AsDijkstra *AsDijkstraPtr;

class AsDijkstra : public AsGeneric {
public:
  AsDijkstra();
  AsDijkstra(int cached);
  ~AsDijkstra();
	xbt_node_t routeGraphNewNode(int id, int graph_id);
	graph_node_map_element_t nodeMapSearch(int id);
	void newRoute(int src_id, int dst_id, sg_platf_route_cbarg_t e_route);
	void addLoopback();
	void getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t route, double *lat);
	xbt_dynar_t getOnelinkRoutes();
	void getRouteAndLatency(sg_platf_route_cbarg_t route, double *lat);
	void parseASroute(sg_platf_route_cbarg_t route);
	void parseRoute(sg_platf_route_cbarg_t route);
	void end();

  xbt_graph_t p_routeGraph;      /* xbt_graph */
  xbt_dict_t p_graphNodeMap;    /* map */
  xbt_dict_t p_routeCache;       /* use in cache mode */
  int m_cached;
};

#endif /* SURF_ROUTING_DIJKSTRA_HPP_ */
