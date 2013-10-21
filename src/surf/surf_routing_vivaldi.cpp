#include "surf_routing_private.h"
#include "surf_routing_vivaldi.hpp"

extern "C" {
  XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_vivaldi, surf, "Routing part of surf");
}

static XBT_INLINE double euclidean_dist_comp(int index, xbt_dynar_t src, xbt_dynar_t dst) {
  double src_coord, dst_coord;

  src_coord = xbt_dynar_get_as(src, index, double);
  dst_coord = xbt_dynar_get_as(dst, index, double);

  return (src_coord-dst_coord)*(src_coord-dst_coord);
}

AS_t model_vivaldi_create(void)
{
  return new AsVivaldi();
}

void AsVivaldi::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t route, double *lat)
{
  s_surf_parsing_link_up_down_t info;

  XBT_DEBUG("vivaldi_get_route_and_latency from '%s'[%d] '%s'[%d]",
		  src->p_name, src->m_id, dst->p_name, dst->m_id);

  if(src->p_rcType == SURF_NETWORK_ELEMENT_AS) {
    route->gw_src = (sg_routing_edge_t) xbt_lib_get_or_null(as_router_lib, ROUTER_PEER(src->p_name), ROUTING_ASR_LEVEL);
    route->gw_dst = (sg_routing_edge_t) xbt_lib_get_or_null(as_router_lib, ROUTER_PEER(dst->p_name), ROUTING_ASR_LEVEL);
  }

  double euclidean_dist;
  xbt_dynar_t src_ctn, dst_ctn;
  char *tmp_src_name, *tmp_dst_name;

  if(src->p_rcType == SURF_NETWORK_ELEMENT_HOST){
    tmp_src_name = HOST_PEER(src->p_name);

    if(p_linkUpDownList){
      info = xbt_dynar_get_as(p_linkUpDownList, src->m_id, s_surf_parsing_link_up_down_t);
      if(info.link_up) { // link up
        xbt_dynar_push_as(route->link_list, void*, info.link_up);
        if (lat)
          *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(info.link_up))->getLatency();
      }
    }
    src_ctn = (xbt_dynar_t) xbt_lib_get_or_null(host_lib, tmp_src_name, COORD_HOST_LEVEL);
    if(!src_ctn ) src_ctn = (xbt_dynar_t) xbt_lib_get_or_null(host_lib, src->p_name, COORD_HOST_LEVEL);
  }
  else if(src->p_rcType == SURF_NETWORK_ELEMENT_ROUTER || src->p_rcType == SURF_NETWORK_ELEMENT_AS){
    tmp_src_name = ROUTER_PEER(src->p_name);
    src_ctn = (xbt_dynar_t) xbt_lib_get_or_null(as_router_lib, tmp_src_name, COORD_ASR_LEVEL);
  }
  else{
    THROW_IMPOSSIBLE;
  }

  if(dst->p_rcType == SURF_NETWORK_ELEMENT_HOST){
    tmp_dst_name = HOST_PEER(dst->p_name);

    if(p_linkUpDownList){
      info = xbt_dynar_get_as(p_linkUpDownList, dst->m_id, s_surf_parsing_link_up_down_t);
      if(info.link_down) { // link down
        xbt_dynar_push_as(route->link_list,void*,info.link_down);
        if (lat)
          *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(info.link_down))->getLatency();
      }
    }
    dst_ctn = (xbt_dynar_t) xbt_lib_get_or_null(host_lib, tmp_dst_name, COORD_HOST_LEVEL);
    if(!dst_ctn ) dst_ctn = (xbt_dynar_t) xbt_lib_get_or_null(host_lib, dst->p_name, COORD_HOST_LEVEL);
  }
  else if(dst->p_rcType == SURF_NETWORK_ELEMENT_ROUTER || dst->p_rcType == SURF_NETWORK_ELEMENT_AS){
    tmp_dst_name = ROUTER_PEER(dst->p_name);
    dst_ctn = (xbt_dynar_t) xbt_lib_get_or_null(as_router_lib, tmp_dst_name, COORD_ASR_LEVEL);
  }
  else{
    THROW_IMPOSSIBLE;
  }

  xbt_assert(src_ctn,"No coordinate found for element '%s'",tmp_src_name);
  xbt_assert(dst_ctn,"No coordinate found for element '%s'",tmp_dst_name);
  free(tmp_src_name);
  free(tmp_dst_name);

  euclidean_dist = sqrt (euclidean_dist_comp(0,src_ctn,dst_ctn)+euclidean_dist_comp(1,src_ctn,dst_ctn))
                      + fabs(xbt_dynar_get_as(src_ctn, 2, double))+fabs(xbt_dynar_get_as(dst_ctn, 2, double));

  if (lat){
    XBT_DEBUG("Updating latency %f += %f",*lat,euclidean_dist);
    *lat += euclidean_dist / 1000.0; //From .ms to .s
  }
}

int AsVivaldi::parsePU(RoutingEdgePtr elm) {
  XBT_DEBUG("Load process unit \"%s\"", elm->p_name);
  xbt_dynar_push_as(p_indexNetworkElm, sg_routing_edge_t, elm);
  return xbt_dynar_length(p_indexNetworkElm)-1;
}
