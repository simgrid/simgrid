/* Copyright (c) 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing.hpp"
#include "surf_routing_private.hpp"
#include "surf_routing_cluster.hpp"

#include "simgrid/platf_interface.h"    // platform creation API internal interface
#include "simgrid/sg_config.h"
#include "storage_interface.hpp"
#include "src/surf/platform.hpp"
#include "surf/surfxml_parse_values.h"

#include "src/surf/surf_routing_cluster_torus.hpp"
#include "src/surf/surf_routing_cluster_fat_tree.hpp"
#include "src/surf/surf_routing_dijkstra.hpp"
#include "src/surf/surf_routing_floyd.hpp"
#include "src/surf/surf_routing_full.hpp"
#include "src/surf/surf_routing_vivaldi.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route, surf, "Routing part of surf");

namespace simgrid {
namespace surf {

  /* Callbacks */
  simgrid::xbt::signal<void(simgrid::surf::NetCard*)> netcardCreatedCallbacks;
  simgrid::xbt::signal<void(simgrid::surf::As*)> asCreatedCallbacks;

  As::As(const char*name)
  : name_(xbt_strdup(name))
  {}
  As::~As()
  {
    xbt_dict_free(&sons_);
    xbt_dynar_free(&vertices_);
    xbt_dynar_free(&upDownLinks);
    xbt_free(name_);
    if (netcard_)
      delete netcard_;
  }

  sg_platf_route_cbarg_t As::getBypassRoute(NetCard * /*src*/, NetCard * /*dst*/, double * /*lat*/) {
    return NULL;
  }


  int As::addComponent(NetCard *elm) {
    XBT_DEBUG("Load component \"%s\"", elm->name());
    xbt_dynar_push_as(vertices_, NetCard*, elm);
    return xbt_dynar_length(vertices_)-1;
  }

  void As::parseRoute(sg_platf_route_cbarg_t /*route*/){
    THROW_IMPOSSIBLE; /* No. */
  }
  void As::parseBypassroute(sg_platf_route_cbarg_t /*e_route*/){
    THROW_IMPOSSIBLE;
  }

}} // namespace simgrid::surf

/**
 * @ingroup SURF_build_api
 * @brief A library containing all known hosts
 */
xbt_dict_t host_list;

int COORD_HOST_LEVEL=0;         //Coordinates level

int MSG_FILE_LEVEL;             //Msg file level

int SIMIX_STORAGE_LEVEL;        //Simix storage level
int MSG_STORAGE_LEVEL;          //Msg storage level

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL;          //Routing level
int COORD_ASR_LEVEL;            //Coordinates level
int NS3_ASR_LEVEL;              //host node for ns3
int ROUTING_PROP_ASR_LEVEL;     //Where the properties are stored

/** @brief Retrieve a netcard from its name
 *
 * Netcards are the thing that connect host or routers to the network
 */
simgrid::surf::NetCard *sg_netcard_by_name_or_null(const char *name)
{
  sg_host_t h = sg_host_by_name(name);
  simgrid::surf::NetCard *net_elm = h==NULL?NULL: h->pimpl_netcard;
  if (!net_elm)
    net_elm = (simgrid::surf::NetCard*) xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL);
  return net_elm;
}

/* Global vars */
simgrid::surf::RoutingPlatf *routing_platf = NULL;


/** The current AS in the parsing */
static simgrid::surf::As *current_routing = NULL;
simgrid::surf::As* routing_get_current()
{
  return current_routing;
}

/** @brief Add a link connecting an host to the rest of its AS (which must be cluster or vivaldi) */
void sg_platf_new_hostlink(sg_platf_host_link_cbarg_t netcard_arg)
{
  simgrid::surf::NetCard *netcard = sg_host_by_name(netcard_arg->id)->pimpl_netcard;
  xbt_assert(netcard, "Host '%s' not found!", netcard_arg->id);
  xbt_assert(dynamic_cast<simgrid::surf::AsCluster*>(current_routing) ||
             dynamic_cast<simgrid::surf::AsVivaldi*>(current_routing),
      "Only hosts from Cluster and Vivaldi ASes can get a host_link.");

  s_surf_parsing_link_up_down_t link_up_down;
  link_up_down.link_up = Link::byName(netcard_arg->link_up);
  link_up_down.link_down = Link::byName(netcard_arg->link_down);

  xbt_assert(link_up_down.link_up, "Link '%s' not found!",netcard_arg->link_up);
  xbt_assert(link_up_down.link_down, "Link '%s' not found!",netcard_arg->link_down);

  // If dynar is is greater than netcard id and if the host_link is already defined
  if((int)xbt_dynar_length(current_routing->upDownLinks) > netcard->id() &&
      xbt_dynar_get_as(current_routing->upDownLinks, netcard->id(), void*))
  surf_parse_error("Host_link for '%s' is already defined!",netcard_arg->id);

  XBT_DEBUG("Push Host_link for host '%s' to position %d", netcard->name(), netcard->id());
  xbt_dynar_set_as(current_routing->upDownLinks, netcard->id(), s_surf_parsing_link_up_down_t, link_up_down);
}

void sg_platf_new_trace(sg_platf_trace_cbarg_t trace)
{
  tmgr_trace_t tmgr_trace;
  if (!trace->file || strcmp(trace->file, "") != 0) {
    tmgr_trace = tmgr_trace_new_from_file(trace->file);
  } else {
    xbt_assert(strcmp(trace->pc_data, ""),
        "Trace '%s' must have either a content, or point to a file on disk.",trace->id);
    tmgr_trace = tmgr_trace_new_from_string(trace->id, trace->pc_data, trace->periodicity);
  }
  xbt_dict_set(traces_set_list, trace->id, (void *) tmgr_trace, NULL);
}

/**
 * \brief Make a new routing component to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host,
 * router or sub-AS) added after this call and before the corresponding call
 * to sg_platf_new_AS_close() will be added to this AS.
 *
 * Once this function was called, the configuration concerning the used
 * models cannot be changed anymore.
 *
 * @param AS_id name of this autonomous system. Must be unique in the platform
 * @param wanted_routing_type one of Full, Floyd, Dijkstra or similar. Full list in the variable routing_models, in src/surf/surf_routing.c
 */
void routing_AS_begin(sg_platf_AS_cbarg_t AS)
{
  XBT_DEBUG("routing_AS_begin");

  xbt_assert(NULL == xbt_lib_get_or_null(as_router_lib, AS->id, ROUTING_ASR_LEVEL),
      "Refusing to create a second AS called \"%s\".", AS->id);

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */


  /* search the routing model */
  simgrid::surf::As *new_as = NULL;
  switch(AS->routing){
    case A_surfxml_AS_routing_Cluster:        new_as = new simgrid::surf::AsCluster(AS->id);        break;
    case A_surfxml_AS_routing_ClusterTorus:   new_as = new simgrid::surf::AsClusterTorus(AS->id);   break;
    case A_surfxml_AS_routing_ClusterFatTree: new_as = new simgrid::surf::AsClusterFatTree(AS->id); break;
    case A_surfxml_AS_routing_Dijkstra:       new_as = new simgrid::surf::AsDijkstra(AS->id, 0);    break;
    case A_surfxml_AS_routing_DijkstraCache:  new_as = new simgrid::surf::AsDijkstra(AS->id, 1);    break;
    case A_surfxml_AS_routing_Floyd:          new_as = new simgrid::surf::AsFloyd(AS->id);          break;
    case A_surfxml_AS_routing_Full:           new_as = new simgrid::surf::AsFull(AS->id);           break;
    case A_surfxml_AS_routing_None:           new_as = new simgrid::surf::AsNone(AS->id);           break;
    case A_surfxml_AS_routing_Vivaldi:        new_as = new simgrid::surf::AsVivaldi(AS->id);        break;
    default:                                  xbt_die("Not a valid model!");                        break;
  }

  /* make a new routing component */
  simgrid::surf::NetCard *netcard = new simgrid::surf::NetCardImpl(new_as->name_, SURF_NETWORK_ELEMENT_AS, current_routing);

  if (current_routing == NULL && routing_platf->p_root == NULL) {
    /* it is the first one */
    new_as->father_ = NULL;
    routing_platf->p_root = new_as;
    netcard->setId(-1);
  } else if (current_routing != NULL && routing_platf->p_root != NULL) {

    xbt_assert(!xbt_dict_get_or_null(current_routing->sons_, AS->id),
               "The AS \"%s\" already exists", AS->id);
    /* it is a part of the tree */
    new_as->father_ = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy_ == SURF_ROUTING_NULL)
      current_routing->hierarchy_ = SURF_ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->sons_, AS->id,
                 (void *) new_as, NULL);
    /* add to the father element list */
    netcard->setId(current_routing->addComponent(netcard));
  } else {
    THROWF(arg_error, 0, "All defined components must belong to a AS");
  }

  xbt_lib_set(as_router_lib, netcard->name(), ROUTING_ASR_LEVEL,
              (void *) netcard);
  XBT_DEBUG("Having set name '%s' id '%d'", new_as->name_, netcard->id());

  /* set the new current component of the tree */
  current_routing = new_as;
  current_routing->netcard_ = netcard;

  simgrid::surf::netcardCreatedCallbacks(netcard);
  simgrid::surf::asCreatedCallbacks(new_as);
}

/**
 * \brief Specify that the current description of AS is finished
 *
 * Once you've declared all the content of your AS, you have to close
 * it with this call. Your AS is not usable until you call this function.
 *
 * @fixme: this call is not as robust as wanted: bad things WILL happen
 * if you call it twice for the same AS, or if you forget calling it, or
 * even if you add stuff to a closed AS
 *
 */
void routing_AS_end()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->Seal();
  current_routing = current_routing->father_;
}

/* Aux Business methods */

/**
 * \brief Get the AS father and the first elements of the chain
 *
 * \param src the source host name
 * \param dst the destination host name
 *
 * Get the common father of the to processing units, and the first different
 * father in the chain
 */
static void elements_father(sg_netcard_t src, sg_netcard_t dst,
                            AS_t * res_father,
                            AS_t * res_src,
                            AS_t * res_dst)
{
  xbt_assert(src && dst, "bad parameters for \"elements_father\" method");
#define ELEMENTS_FATHER_MAXDEPTH 16     /* increase if it is not enough */
  simgrid::surf::As *src_as, *dst_as;
  simgrid::surf::As *path_src[ELEMENTS_FATHER_MAXDEPTH];
  simgrid::surf::As *path_dst[ELEMENTS_FATHER_MAXDEPTH];
  int index_src = 0;
  int index_dst = 0;
  simgrid::surf::As *current;
  simgrid::surf::As *current_src;
  simgrid::surf::As *current_dst;
  simgrid::surf::As *father;

  /* (1) find the as where the src and dst are located */
  sg_netcard_t src_data = src;
  sg_netcard_t dst_data = dst;
  src_as = src_data->containingAS();
  dst_as = dst_data->containingAS();
#ifndef NDEBUG
  char* src_name = src_data->name();
  char* dst_name = dst_data->name();
#endif

  xbt_assert(src_as && dst_as,
             "Ask for route \"from\"(%s) or \"to\"(%s) no found", src_name, dst_name);

  /* (2) find the path to the root routing component */
  for (current = src_as; current != NULL; current = current->father_) {
    if (index_src >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_src");
    path_src[index_src++] = current;
  }
  for (current = dst_as; current != NULL; current = current->father_) {
    if (index_dst >= ELEMENTS_FATHER_MAXDEPTH)
      xbt_die("ELEMENTS_FATHER_MAXDEPTH should be increased for path_dst");
    path_dst[index_dst++] = current;
  }

  /* (3) find the common father */
  do {
    current_src = path_src[--index_src];
    current_dst = path_dst[--index_dst];
  } while (index_src > 0 && index_dst > 0 && current_src == current_dst);

  /* (4) they are not in the same routing component, make the path */
  if (current_src == current_dst)
    father = current_src;
  else
    father = path_src[index_src + 1];

  /* (5) result generation */
  *res_father = father;         /* first the common father of src and dst */
  *res_src = current_src;       /* second the first different father of src */
  *res_dst = current_dst;       /* three  the first different father of dst */

#undef ELEMENTS_FATHER_MAXDEPTH
}

/* Global Business methods */

/**
 * \brief Recursive function for get_route_latency
 *
 * \param src the source host name
 * \param dst the destination host name
 * \param *route the route where the links are stored. It is either NULL or a ready to use dynar
 * \param *latency the latency, if needed
 *
 * This function is called by "get_route" and "get_latency". It allows to walk
 * recursively through the ASes tree.
 */
static void _get_route_and_latency(
  simgrid::surf::NetCard *src, simgrid::surf::NetCard *dst,
  xbt_dynar_t * links, double *latency)
{
  s_sg_platf_route_cbarg_t route = SG_PLATF_ROUTE_INITIALIZER;
  memset(&route,0,sizeof(route));

  xbt_assert(src && dst, "bad parameters for \"_get_route_latency\" method");
  XBT_DEBUG("Solve route/latency  \"%s\" to \"%s\"", src->name(), dst->name());

  /* Find how src and dst are interconnected */
  simgrid::surf::As *common_father, *src_father, *dst_father;
  elements_father(src, dst, &common_father, &src_father, &dst_father);
  XBT_DEBUG("elements_father: common father '%s' src_father '%s' dst_father '%s'",
      common_father->name_, src_father->name_, dst_father->name_);

  /* Check whether a direct bypass is defined */
  sg_platf_route_cbarg_t e_route_bypass = NULL;
  //FIXME:REMOVE:if (common_father->get_bypass_route)

  e_route_bypass = common_father->getBypassRoute(src, dst, latency);

  /* Common ancestor is kind enough to declare a bypass route from src to dst -- use it and bail out */
  if (e_route_bypass) {
    xbt_dynar_merge(links, &e_route_bypass->link_list);
    routing_route_free(e_route_bypass);
    return;
  }

  /* If src and dst are in the same AS, life is good */
  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */
    route.link_list = *links;
    common_father->getRouteAndLatency(src, dst, &route, latency);
    // if vivaldi latency+=vivaldi(src,dst)
    return;
  }

  /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

  route.link_list = xbt_dynar_new(sizeof(sg_routing_link_t), NULL);
  // Find the net_card corresponding to father
  simgrid::surf::NetCard *src_father_netcard = src_father->netcard_;
  simgrid::surf::NetCard *dst_father_netcard = dst_father->netcard_;

  common_father->getRouteAndLatency(src_father_netcard, dst_father_netcard,
                                    &route, latency);

  xbt_assert((route.gw_src != NULL) && (route.gw_dst != NULL),
      "bad gateways for route from \"%s\" to \"%s\"", src->name(), dst->name());

  sg_netcard_t src_gateway_net_elm = route.gw_src;
  sg_netcard_t dst_gateway_net_elm = route.gw_dst;

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != src_gateway_net_elm)
    _get_route_and_latency(src, src_gateway_net_elm, links, latency);
  xbt_dynar_merge(links, &route.link_list);

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  if (dst_gateway_net_elm != dst)
    _get_route_and_latency(dst_gateway_net_elm, dst, links, latency);

}

AS_t surf_platf_get_root(routing_platf_t platf){
  return platf->p_root;
}

e_surf_network_element_type_t surf_routing_edge_get_rc_type(sg_netcard_t netcard){
  return netcard->getRcType();
}

namespace simgrid {
namespace surf {

/**
 * \brief Find a route between hosts
 *
 * \param src the network_element_t for src host
 * \param dst the network_element_t for dst host
 * \param route where to store the list of links.
 *              If *route=NULL, create a short lived dynar. Else, fill the provided dynar
 * \param latency where to store the latency experienced on the path (or NULL if not interested)
 *                It is the caller responsability to initialize latency to 0 (we add to provided route)
 * \pre route!=NULL
 *
 * walk through the routing components tree and find a route between hosts
 * by calling each "get_route" function in each routing component.
 */
void RoutingPlatf::getRouteAndLatency(NetCard *src, NetCard *dst, xbt_dynar_t* route, double *latency)
{
  XBT_DEBUG("getRouteAndLatency from %s to %s", src->name(), dst->name());
  if (NULL == *route) {
    xbt_dynar_reset(routing_platf->p_lastRoute);
    *route = routing_platf->p_lastRoute;
  }

  _get_route_and_latency(src, dst, route, latency);

  xbt_assert(!latency || *latency >= 0.0,
             "negative latency on route between \"%s\" and \"%s\"", src->name(), dst->name());
}

xbt_dynar_t RoutingPlatf::getOneLinkRoutes(){
  return recursiveGetOneLinkRoutes(p_root);
}

xbt_dynar_t RoutingPlatf::recursiveGetOneLinkRoutes(As *rc)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(Onelink*), xbt_free_f);

  //adding my one link routes
  xbt_dynar_t onelink_mine = rc->getOneLinkRoutes();
  if (onelink_mine)
    xbt_dynar_merge(&ret,&onelink_mine);

  //recursing
  char *key;
  xbt_dict_cursor_t cursor = NULL;
  AS_t rc_child;
  xbt_dict_foreach(rc->sons_, cursor, key, rc_child) {
    xbt_dynar_t onelink_child = recursiveGetOneLinkRoutes(rc_child);
    if (onelink_child)
      xbt_dynar_merge(&ret,&onelink_child);
  }
  return ret;
}

}
}

e_surf_network_element_type_t routing_get_network_element_type(const char *name)
{
  simgrid::surf::NetCard *rc = sg_netcard_by_name_or_null(name);
  if (rc)
    return rc->getRcType();

  return SURF_NETWORK_ELEMENT_NULL;
}

/** @brief create the root AS */
void routing_model_create( void *loopback)
{
  routing_platf = new simgrid::surf::RoutingPlatf(loopback);
}

/* ************************************************************************** */
/* ************************* GENERIC PARSE FUNCTIONS ************************ */

void routing_cluster_add_backbone(simgrid::surf::Link* bb) {
  simgrid::surf::AsCluster *cluster = dynamic_cast<simgrid::surf::AsCluster*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(nullptr == cluster->p_backbone, "Cluster %s already has a backbone link!", cluster->name_);

  cluster->p_backbone = bb;
  XBT_DEBUG("Add a backbone to AS '%s'", current_routing->name_);
}

void sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet)
{
  int start, end, i;
  char *groups , *host_id , *link_id = NULL;
  unsigned int iter;
  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;

  //Make all hosts
  radical_elements = xbt_str_split(cabinet->radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {

    radical_ends = xbt_str_split(groups, "-");
    start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));

    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      end = start;
      break;
    case 2:
      end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
      break;
    default:
      surf_parse_error("Malformed radical");
      break;
    }
    s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
    memset(&host, 0, sizeof(host));
    host.initiallyOn   = 1;
    host.pstate        = 0;
    host.speed_scale   = 1.0;
    host.core_amount   = 1;

    s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
    memset(&link, 0, sizeof(link));
    link.initiallyOn = 1;
    link.policy    = SURF_LINK_FULLDUPLEX;
    link.latency   = cabinet->lat;
    link.bandwidth = cabinet->bw;

    s_sg_platf_host_link_cbarg_t host_link = SG_PLATF_HOST_LINK_INITIALIZER;
    memset(&host_link, 0, sizeof(host_link));

    for (i = start; i <= end; i++) {
      host_id                      = bprintf("%s%d%s",cabinet->prefix,i,cabinet->suffix);
      link_id                      = bprintf("link_%s%d%s",cabinet->prefix,i,cabinet->suffix);
      host.id                      = host_id;
      link.id                      = link_id;
      host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
      xbt_dynar_push(host.speed_peak,&cabinet->speed);
      sg_platf_new_host(&host);
      xbt_dynar_free(&host.speed_peak);
      sg_platf_new_link(&link);

      char* link_up       = bprintf("%s_UP",link_id);
      char* link_down     = bprintf("%s_DOWN",link_id);
      host_link.id        = host_id;
      host_link.link_up   = link_up;
      host_link.link_down = link_down;
      sg_platf_new_hostlink(&host_link);

      free(host_id);
      free(link_id);
      free(link_up);
      free(link_down);
    }

    xbt_dynar_free(&radical_ends);
  }
  xbt_dynar_free(&radical_elements);
}

void sg_platf_new_peer(sg_platf_peer_cbarg_t peer)
{
  using simgrid::surf::NetCard;
  using simgrid::surf::AsCluster;

  char *host_id = NULL;
  char *link_id = NULL;
  char *router_id = NULL;

  XBT_DEBUG(" ");
  host_id = HOST_PEER(peer->id);
  link_id = LINK_PEER(peer->id);
  router_id = ROUTER_PEER(peer->id);

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Cluster\">", peer->id);
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  AS.id                    = peer->id;
  AS.routing               = A_surfxml_AS_routing_Cluster;
  sg_platf_new_AS_begin(&AS);

  XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\"/>", host_id, peer->speed);
  s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
  memset(&host, 0, sizeof(host));
  host.initiallyOn = 1;
  host.id = host_id;

  host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
  xbt_dynar_push(host.speed_peak,&peer->speed);
  host.pstate = 0;
  //host.power_peak = peer->power;
  host.speed_scale = 1.0;
  host.speed_trace = peer->availability_trace;
  host.state_trace = peer->state_trace;
  host.core_amount = 1;
  sg_platf_new_host(&host);
  xbt_dynar_free(&host.speed_peak);

  s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
  memset(&link, 0, sizeof(link));
  link.initiallyOn = 1;
  link.policy  = SURF_LINK_SHARED;
  link.latency = peer->lat;

  char* link_up = bprintf("%s_UP",link_id);
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_up,
            peer->bw_out, peer->lat);
  link.id = link_up;
  link.bandwidth = peer->bw_out;
  sg_platf_new_link(&link);

  char* link_down = bprintf("%s_DOWN",link_id);
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_down,
            peer->bw_in, peer->lat);
  link.id = link_down;
  link.bandwidth = peer->bw_in;
  sg_platf_new_link(&link);

  XBT_DEBUG("<host_link\tid=\"%s\"\tup=\"%s\"\tdown=\"%s\" />", host_id,link_up,link_down);
  s_sg_platf_host_link_cbarg_t host_link = SG_PLATF_HOST_LINK_INITIALIZER;
  memset(&host_link, 0, sizeof(host_link));
  host_link.id        = host_id;
  host_link.link_up   = link_up;
  host_link.link_down = link_down;
  sg_platf_new_hostlink(&host_link);

  XBT_DEBUG("<router id=\"%s\"/>", router_id);
  s_sg_platf_router_cbarg_t router = SG_PLATF_ROUTER_INITIALIZER;
  memset(&router, 0, sizeof(router));
  router.id = router_id;
  router.coord = peer->coord;
  sg_platf_new_router(&router);
  static_cast<AsCluster*>(current_routing)->p_router = static_cast<NetCard*>(xbt_lib_get_or_null(as_router_lib, router.id, ROUTING_ASR_LEVEL));

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");

  //xbt_dynar_free(&tab_elements_num);
  free(router_id);
  free(host_id);
  free(link_id);
  free(link_up);
  free(link_down);
}

// static void routing_parse_Srandom(void)
// {
//   double mean, std, min, max, seed;
//   char *random_id = A_surfxml_random_id;
//   char *random_radical = A_surfxml_random_radical;
//   char *rd_name = NULL;
//   char *rd_value;
//   mean = surf_parse_get_double(A_surfxml_random_mean);
//   std = surf_parse_get_double(A_surfxml_random_std___deviation);
//   min = surf_parse_get_double(A_surfxml_random_min);
//   max = surf_parse_get_double(A_surfxml_random_max);
//   seed = surf_parse_get_double(A_surfxml_random_seed);

//   double res = 0;
//   int i = 0;
//   random_data_t random = xbt_new0(s_random_data_t, 1);
//   char *tmpbuf;

//   xbt_dynar_t radical_elements;
//   unsigned int iter;
//   char *groups;
//   int start, end;
//   xbt_dynar_t radical_ends;

//   switch (A_surfxml_random_generator) {
//   case AU_surfxml_random_generator:
//   case A_surfxml_random_generator_NONE:
//     random->generator = NONE;
//     break;
//   case A_surfxml_random_generator_DRAND48:
//     random->generator = DRAND48;
//     break;
//   case A_surfxml_random_generator_RAND:
//     random->generator = RAND;
//     break;
//   case A_surfxml_random_generator_RNGSTREAM:
//     random->generator = RNGSTREAM;
//     break;
//   default:
//     surf_parse_error("Invalid random generator");
//     break;
//   }
//   random->seed = seed;
//   random->min = min;
//   random->max = max;

//   /* Check user stupidities */
//   if (max < min)
//     THROWF(arg_error, 0, "random->max < random->min (%f < %f)", max, min);
//   if (mean < min)
//     THROWF(arg_error, 0, "random->mean < random->min (%f < %f)", mean, min);
//   if (mean > max)
//     THROWF(arg_error, 0, "random->mean > random->max (%f > %f)", mean, max);

//   /* normalize the mean and standard deviation before storing */
//   random->mean = (mean - min) / (max - min);
//   random->std = std / (max - min);

//   if (random->mean * (1 - random->mean) < random->std * random->std)
//     THROWF(arg_error, 0, "Invalid mean and standard deviation (%f and %f)",
//            random->mean, random->std);

//   XBT_DEBUG
//       ("id = '%s' min = '%f' max = '%f' mean = '%f' std_deviatinon = '%f' generator = '%d' seed = '%ld' radical = '%s'",
//        random_id, random->min, random->max, random->mean, random->std,
//        (int)random->generator, random->seed, random_radical);

//   if (!random_value)
//     random_value = xbt_dict_new_homogeneous(free);

//   if (!strcmp(random_radical, "")) {
//     res = random_generate(random);
//     rd_value = bprintf("%f", res);
//     xbt_dict_set(random_value, random_id, rd_value, NULL);
//   } else {
//     radical_elements = xbt_str_split(random_radical, ",");
//     xbt_dynar_foreach(radical_elements, iter, groups) {
//       radical_ends = xbt_str_split(groups, "-");
//       switch (xbt_dynar_length(radical_ends)) {
//       case 1:
//         xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
//                    "Custom Random '%s' already exists !", random_id);
//         res = random_generate(random);
//         tmpbuf =
//             bprintf("%s%d", random_id,
//                     atoi(xbt_dynar_getfirst_as(radical_ends, char *)));
//         xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), NULL);
//         xbt_free(tmpbuf);
//         break;

//       case 2:
//         start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
//         end = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 1, char *));
//         for (i = start; i <= end; i++) {
//           xbt_assert(!xbt_dict_get_or_null(random_value, random_id),
//                      "Custom Random '%s' already exists !", bprintf("%s%d",
//                                                                     random_id,
//                                                                     i));
//           res = random_generate(random);
//           tmpbuf = bprintf("%s%d", random_id, i);
//           xbt_dict_set(random_value, tmpbuf, bprintf("%f", res), NULL);
//           xbt_free(tmpbuf);
//         }
//         break;
//       default:
//         XBT_CRITICAL("Malformed radical");
//         break;
//       }
//       res = random_generate(random);
//       rd_name = bprintf("%s_router", random_id);
//       rd_value = bprintf("%f", res);
//       xbt_dict_set(random_value, rd_name, rd_value, NULL);

//       xbt_dynar_free(&radical_ends);
//     }
//     free(rd_name);
//     xbt_dynar_free(&radical_elements);
//   }
// }

static void check_disk_attachment()
{
  xbt_lib_cursor_t cursor;
  char *key;
  void **data;
  simgrid::surf::NetCard *host_elm;
  xbt_lib_foreach(storage_lib, cursor, key, data) {
    if(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL) != NULL) {
    simgrid::surf::Storage *storage = static_cast<simgrid::surf::Storage*>(xbt_lib_get_level(xbt_lib_get_elm_or_null(storage_lib, key), SURF_STORAGE_LEVEL));
    host_elm = sg_netcard_by_name_or_null(storage->p_attach);
    if(!host_elm)
      surf_parse_error("Unable to attach storage %s: host %s doesn't exist.", storage->getName(), storage->p_attach);
    }
  }
}

void routing_register_callbacks()
{
  simgrid::surf::on_postparse.connect(check_disk_attachment);

  instr_routing_define_callbacks();
}

/**
 * \brief Recursive function for finalize
 *
 * \param rc the source host name
 *
 * This fuction is call by "finalize". It allow to finalize the
 * AS or routing components. It delete all the structures.
 */
static void finalize_rec(simgrid::surf::As *as) {
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;

  xbt_dict_foreach(as->sons_, cursor, key, elem) {
    finalize_rec(elem);
  }

  delete as;;
}

/** \brief Frees all memory allocated by the routing module */
void routing_exit(void) {
  delete routing_platf;
}

namespace simgrid {
namespace surf {

  RoutingPlatf::RoutingPlatf(void *loopback)
  : p_loopback(loopback)
  {
  }
  RoutingPlatf::~RoutingPlatf()
  {
    xbt_dynar_free(&p_lastRoute);
    finalize_rec(p_root);
  }

}
}

AS_t surf_AS_get_routing_root() {
  return routing_platf->p_root;
}

const char *surf_AS_get_name(simgrid::surf::As *as) {
  return as->name_;
}

static simgrid::surf::As *surf_AS_recursive_get_by_name(
  simgrid::surf::As *current, const char * name)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;
  simgrid::surf::As *tmp = NULL;

  if(!strcmp(current->name_, name))
    return current;

  xbt_dict_foreach(current->sons_, cursor, key, elem) {
    tmp = surf_AS_recursive_get_by_name(elem, name);
    if(tmp != NULL ) {
        break;
    }
  }
  return tmp;
}

simgrid::surf::As *surf_AS_get_by_name(const char * name)
{
  simgrid::surf::As *as = surf_AS_recursive_get_by_name(routing_platf->p_root, name);
  if(as == NULL)
    XBT_WARN("Impossible to find an AS with name %s, please check your input", name);
  return as;
}

xbt_dict_t surf_AS_get_routing_sons(simgrid::surf::As *as)
{
  return as->sons_;
}

xbt_dynar_t surf_AS_get_hosts(simgrid::surf::As *as)
{
  xbt_dynar_t elms = as->vertices_;
  int count = xbt_dynar_length(elms);
  xbt_dynar_t res =  xbt_dynar_new(sizeof(sg_host_t), NULL);
  for (int index = 0; index < count; index++) {
     sg_netcard_t relm =
      xbt_dynar_get_as(elms, index, simgrid::surf::NetCard*);
     sg_host_t delm = simgrid::s4u::Host::by_name_or_null(relm->name());
     if (delm!=NULL) {
       xbt_dynar_push(res, &delm);
     }
  }
  return res;
}

void surf_AS_get_graph(AS_t as, xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges) {
  as->getGraph(graph, nodes, edges);
}
