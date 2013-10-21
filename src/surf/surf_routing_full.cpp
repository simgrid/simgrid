#include "surf_routing_private.h"
#include "surf_routing_full.hpp"
#include "network.hpp"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_full, surf, "Routing part of surf");
}

/* Global vars */
extern routing_platf_t routing_platf;
extern int surf_parse_lineno;

#define TO_ROUTE_FULL(i,j) p_routingTable[(i)+(j)*table_size]

AS_t model_full_create(void)
{
  return new AsFull();
}

void model_full_end(AS_t _routing)
{
  unsigned int i;
  sg_platf_route_cbarg_t e_route;

  /* set utils vars */
  AsFullPtr routing = ((AsFullPtr) _routing);
  size_t table_size = xbt_dynar_length(routing->p_indexNetworkElm);

  /* Create table if necessary */
  if (!routing->p_routingTable)
    routing->p_routingTable = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  /* Add the loopback if needed */
  if (routing_platf->p_loopback && routing->p_hierarchy == SURF_ROUTING_BASE) {
    for (i = 0; i < table_size; i++) {
      e_route = routing->TO_ROUTE_FULL(i, i);
      if (!e_route) {
        e_route = xbt_new0(s_sg_platf_route_cbarg_t, 1);
        e_route->gw_src = NULL;
        e_route->gw_dst = NULL;
        e_route->link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
        xbt_dynar_push(e_route->link_list, &routing_platf->p_loopback);
        routing->TO_ROUTE_FULL(i, i) = e_route;
      }
    }
  }
}

AsFull::AsFull(){
}

AsFull::~AsFull(){
  size_t table_size = xbt_dynar_length(p_indexNetworkElm);
  int i, j;
  /* Delete routing table */
  for (i = 0; i < table_size; i++)
    for (j = 0; j < table_size; j++)
      delete TO_ROUTE_FULL(i,j);
  xbt_free(p_routingTable);
}

xbt_dynar_t AsFull::getOneLinkRoutes()
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(onelink_t), xbt_free);

  int src, dst;
  int table_size = xbt_dynar_length(p_indexNetworkElm);

  for(src=0; src < table_size; src++) {
    for(dst=0; dst< table_size; dst++) {
      sg_platf_route_cbarg_t route = TO_ROUTE_FULL(src,dst);
      if (route) {
        if (xbt_dynar_length(route->link_list) == 1) {
          void *link = *(void **) xbt_dynar_get_ptr(route->link_list, 0);
          onelink_t onelink = xbt_new0(s_onelink_t, 1);
          onelink->link_ptr = link;
          if (p_hierarchy == SURF_ROUTING_BASE) {
            onelink->src = xbt_dynar_get_as(p_indexNetworkElm, src, sg_routing_edge_t);
            onelink->src->m_id = src;
            onelink->dst = xbt_dynar_get_as(p_indexNetworkElm, dst, sg_routing_edge_t);
            onelink->dst->m_id = dst;
          } else if (p_hierarchy == SURF_ROUTING_RECURSIVE) {
            onelink->src = route->gw_src;
            onelink->dst = route->gw_dst;
          }
          xbt_dynar_push(ret, &onelink);
          XBT_DEBUG("Push route from '%d' to '%d'",
              src,
              dst);
        }
      }
    }
  }
  return ret;
}

void AsFull::getRouteAndLatency(RoutingEdgePtr src, RoutingEdgePtr dst, sg_platf_route_cbarg_t res, double *lat)
{
  XBT_DEBUG("full_get_route_and_latency from %s[%d] to %s[%d]",
      src->p_name,
      src->m_id,
      dst->p_name,
      dst->m_id  );

  /* set utils vars */
  size_t table_size = xbt_dynar_length(p_indexNetworkElm);

  sg_platf_route_cbarg_t e_route = NULL;
  void *link;
  unsigned int cpt = 0;

  e_route = TO_ROUTE_FULL(src->m_id, dst->m_id);

  if (e_route) {
    res->gw_src = e_route->gw_src;
    res->gw_dst = e_route->gw_dst;
    xbt_dynar_foreach(e_route->link_list, cpt, link) {
      xbt_dynar_push(res->link_list, &link);
      if (lat)
        *lat += dynamic_cast<NetworkCm02LinkPtr>(static_cast<ResourcePtr>(link))->getLatency();
    }
  }
}

void AsFull::parseASroute(sg_platf_route_cbarg_t route){
  parseRoute(route);
}

static int full_pointer_resource_cmp(const void *a, const void *b)
{
  return a != b;
}

void AsFull::parseRoute(sg_platf_route_cbarg_t route)
{
  int as_route = 0;
  char *src = (char*)(route->src);
  char *dst = (char*)(route->dst);
  RoutingEdgePtr src_net_elm, dst_net_elm;
  src_net_elm = sg_routing_edge_by_name_or_null(src);
  dst_net_elm = sg_routing_edge_by_name_or_null(dst);

  xbt_assert(src_net_elm, "Network elements %s not found", src);
  xbt_assert(dst_net_elm, "Network elements %s not found", dst);

  size_t table_size = xbt_dynar_length(p_indexNetworkElm);

  xbt_assert(!xbt_dynar_is_empty(route->link_list),
      "Invalid count of links, must be greater than zero (%s,%s)",
      src, dst);

  if (!p_routingTable)
    p_routingTable = xbt_new0(sg_platf_route_cbarg_t, table_size * table_size);

  if (TO_ROUTE_FULL(src_net_elm->m_id, dst_net_elm->m_id)) {
    char *link_name;
    unsigned int i;
    xbt_dynar_t link_route_to_test =
        xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
    xbt_dynar_foreach(route->link_list, i, link_name) {
      void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
      xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
      xbt_dynar_push(link_route_to_test, &link);
    }
    if (xbt_dynar_compare(TO_ROUTE_FULL(src_net_elm->m_id, dst_net_elm->m_id)->link_list,
        link_route_to_test, full_pointer_resource_cmp)) {
      surf_parse_error("A route between \"%s\" and \"%s\" already exists "
          "with a different content. "
          "If you are trying to define a reverse route, "
          "you must set the symmetrical=no attribute to "
          "your routes tags.", src, dst);
    } else {
      surf_parse_warn("Ignoring the identical redefinition of the route "
          "between \"%s\" and \"%s\"", src, dst);
    }
  } else {
    if (!route->gw_dst && !route->gw_dst)
      XBT_DEBUG("Load Route from \"%s\" to \"%s\"", src, dst);
    else {
      // FIXME We can call a gw which is down the current AS (cf g5k.xml) but not upper.
      //      AS_t subas = xbt_dict_get_or_null(rc->routing_sons, src);
      //      if (subas == NULL)
      //        surf_parse_error("The source of an ASroute must be a sub-AS "
      //                         "declared within the current AS, "
      //                         "but '%s' is not an AS within '%s'", src, rc->name);
      //      if (subas->to_index
      //          && xbt_dict_get_or_null(subas->to_index, route->src_gateway) == NULL)
      //        surf_parse_error("In an ASroute, source gateway must be part of "
      //                         "the source sub-AS (in particular, being in a "
      //                         "sub-sub-AS is not allowed), "
      //                         "but '%s' is not in '%s'.",
      //                         route->src_gateway, subas->name);
      //
      //      subas = xbt_dict_get_or_null(rc->routing_sons, dst);
      //      if (subas == NULL)
      //        surf_parse_error("The destination of an ASroute must be a sub-AS "
      //                         "declared within the current AS, "
      //                         "but '%s' is not an AS within '%s'", dst, rc->name);
      //      if (subas->to_index
      //          && xbt_dict_get_or_null(subas->to_index, route->dst_gateway) == NULL)
      //        surf_parse_error("In an ASroute, destination gateway must be "
      //                         "part of the destination sub-AS (in particular, "
      //                         "in a sub-sub-AS is not allowed), "
      //                         "but '%s' is not in '%s'.",
      //                         route->dst_gateway, subas->name);
      as_route = 1;
      XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
          src, route->gw_src->p_name, dst, route->gw_dst->p_name);
      if (route->gw_dst->p_rcType == SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The dst_gateway '%s' does not exist!", route->gw_dst->p_name);
      if (route->gw_src->p_rcType == SURF_NETWORK_ELEMENT_NULL)
        xbt_die("The src_gateway '%s' does not exist!", route->gw_src->p_name);
    }
    TO_ROUTE_FULL(src_net_elm->m_id, dst_net_elm->m_id) = newExtendedRoute(p_hierarchy, route, 1);
    xbt_dynar_shrink(TO_ROUTE_FULL(src_net_elm->m_id, dst_net_elm->m_id)->link_list, 0);
  }

  if ( (route->symmetrical == TRUE && as_route == 0)
      || (route->symmetrical == TRUE && as_route == 1)
  ) {
    if (route->gw_dst && route->gw_src) {
      sg_routing_edge_t gw_tmp;
      gw_tmp = route->gw_src;
      route->gw_src = route->gw_dst;
      route->gw_dst = gw_tmp;
    }
    if (TO_ROUTE_FULL(dst_net_elm->m_id, src_net_elm->m_id)) {
      char *link_name;
      unsigned int i;
      xbt_dynar_t link_route_to_test =
          xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
      for (i = xbt_dynar_length(route->link_list); i > 0; i--) {
        link_name = xbt_dynar_get_as(route->link_list, i - 1, char *);
        void *link = xbt_lib_get_or_null(link_lib, link_name, SURF_LINK_LEVEL);
        xbt_assert(link, "Link : '%s' doesn't exists.", link_name);
        xbt_dynar_push(link_route_to_test, &link);
      }
      xbt_assert(!xbt_dynar_compare(TO_ROUTE_FULL(dst_net_elm->m_id, src_net_elm->m_id)->link_list,
          link_route_to_test,
          full_pointer_resource_cmp),
          "The route between \"%s\" and \"%s\" already exists", src,
          dst);
    } else {
      if (!route->gw_dst && !route->gw_src)
        XBT_DEBUG("Load Route from \"%s\" to \"%s\"", dst, src);
      else
        XBT_DEBUG("Load ASroute from \"%s(%s)\" to \"%s(%s)\"",
            dst, route->gw_src->p_name, src, route->gw_dst->p_name);
      TO_ROUTE_FULL(dst_net_elm->m_id, src_net_elm->m_id) = newExtendedRoute(p_hierarchy, route, 0);
      xbt_dynar_shrink(TO_ROUTE_FULL(dst_net_elm->m_id, src_net_elm->m_id)->link_list, 0);
    }
  }
  xbt_dynar_free(&route->link_list);
}




