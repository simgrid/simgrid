/* Copyright (c) 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_routing.hpp"
#include "surf_routing_cluster.hpp"

#include "simgrid/sg_config.h"
#include "storage_interface.hpp"

#include "src/surf/surf_routing_cluster_torus.hpp"
#include "src/surf/surf_routing_cluster_fat_tree.hpp"
#include "src/surf/surf_routing_dijkstra.hpp"
#include "src/surf/surf_routing_floyd.hpp"
#include "src/surf/surf_routing_full.hpp"
#include "src/surf/surf_routing_vivaldi.hpp"
#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area

#include <vector>

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
    xbt_dict_cursor_t cursor = NULL;
    char *key;
    AS_t elem;
    xbt_dict_foreach(sons_, cursor, key, elem) {
      delete (As*)elem;
    }


    xbt_dict_free(&sons_);
    xbt_dynar_free(&vertices_);
    xbt_dynar_free(&upDownLinks);
    for (auto &kv : bypassRoutes_)
      delete kv.second;
    xbt_free(name_);
    delete netcard_;
  }
  void As::Seal()
  {
    sealed_ = true;
  }

  xbt_dynar_t As::getOneLinkRoutes() {
    return NULL;
  }

  int As::addComponent(NetCard *elm) {
    XBT_DEBUG("Load component \"%s\"", elm->name());
    xbt_dynar_push_as(vertices_, NetCard*, elm);
    return xbt_dynar_length(vertices_)-1;
  }

  void As::addRoute(sg_platf_route_cbarg_t /*route*/){
    xbt_die("AS %s does not accept new routes (wrong class).",name_);
  }

  std::vector<Link*> *As::getBypassRoute(NetCard *src, NetCard *dst)
  {
    // If never set a bypass route return NULL without any further computations
    XBT_DEBUG("generic_get_bypassroute from %s to %s", src->name(), dst->name());
    if (bypassRoutes_.empty())
      return nullptr;

    std::vector<Link*> *bypassedRoute = nullptr;

    if(dst->containingAS() == this && src->containingAS() == this ){
      char *route_name = bprintf("%s#%s", src->name(), dst->name());
      if (bypassRoutes_.find(route_name) != bypassRoutes_.end()) {
        bypassedRoute = bypassRoutes_.at(route_name);
        XBT_DEBUG("Found a bypass route with %zu links",bypassedRoute->size());
      }
      free(route_name);
      return bypassedRoute;
    }

    int index_src, index_dst;
    As **current_src = NULL;
    As **current_dst = NULL;

    As *src_as = src->containingAS();
    As *dst_as = dst->containingAS();

    /* (2) find the path to the root routing component */
    xbt_dynar_t path_src = xbt_dynar_new(sizeof(As*), NULL);
    As *current = src_as;
    while (current != NULL) {
      xbt_dynar_push(path_src, &current);
      current = current->father_;
    }
    xbt_dynar_t path_dst = xbt_dynar_new(sizeof(As*), NULL);
    current = dst_as;
    while (current != NULL) {
      xbt_dynar_push(path_dst, &current);
      current = current->father_;
    }

    /* (3) find the common father */
    index_src = path_src->used - 1;
    index_dst = path_dst->used - 1;
    current_src = (As **) xbt_dynar_get_ptr(path_src, index_src);
    current_dst = (As **) xbt_dynar_get_ptr(path_dst, index_dst);
    while (index_src >= 0 && index_dst >= 0 && *current_src == *current_dst) {
      xbt_dynar_pop_ptr(path_src);
      xbt_dynar_pop_ptr(path_dst);
      index_src--;
      index_dst--;
      current_src = (As **) xbt_dynar_get_ptr(path_src, index_src);
      current_dst = (As **) xbt_dynar_get_ptr(path_dst, index_dst);
    }

    int max_index_src = path_src->used - 1;
    int max_index_dst = path_dst->used - 1;

    int max_index = std::max(max_index_src, max_index_dst);

    for (int max = 0; max <= max_index; max++) {
      for (int i = 0; i < max; i++) {
        if (i <= max_index_src && max <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(As **) (xbt_dynar_get_ptr(path_src, i)))->name_,
              (*(As **) (xbt_dynar_get_ptr(path_dst, max)))->name_);
          if (bypassRoutes_.find(route_name) != bypassRoutes_.end())
            bypassedRoute = bypassRoutes_.at(route_name);
          xbt_free(route_name);
        }
        if (bypassedRoute)
          break;
        if (max <= max_index_src && i <= max_index_dst) {
          char *route_name = bprintf("%s#%s",
              (*(As **) (xbt_dynar_get_ptr(path_src, max)))->name_,
              (*(As **) (xbt_dynar_get_ptr(path_dst, i)))->name_);
          if (bypassRoutes_.find(route_name) != bypassRoutes_.end())
            bypassedRoute = bypassRoutes_.at(route_name);
          xbt_free(route_name);
        }
        if (bypassedRoute)
          break;
      }

      if (bypassedRoute)
        break;

      if (max <= max_index_src && max <= max_index_dst) {
        char *route_name = bprintf("%s#%s",
            (*(As **) (xbt_dynar_get_ptr(path_src, max)))->name_,
            (*(As **) (xbt_dynar_get_ptr(path_dst, max)))->name_);

        if (bypassRoutes_.find(route_name) != bypassRoutes_.end())
          bypassedRoute = bypassRoutes_.at(route_name);
        xbt_free(route_name);
      }
      if (bypassedRoute)
        break;
    }

    xbt_dynar_free(&path_src);
    xbt_dynar_free(&path_dst);

    return bypassedRoute;
  }

  void As::addBypassRoute(sg_platf_route_cbarg_t e_route){
    const char *src = e_route->src;
    const char *dst = e_route->dst;

    char *route_name = bprintf("%s#%s", src, dst);

    /* Argument validity checks */
    if (e_route->gw_dst) {
      XBT_DEBUG("Load bypassASroute from %s@%s to %s@%s",
          src, e_route->gw_src->name(), dst, e_route->gw_dst->name());
      xbt_assert(!e_route->link_list->empty(), "Bypass route between %s@%s and %s@%s cannot be empty.",
          src, e_route->gw_src->name(), dst, e_route->gw_dst->name());
      xbt_assert(bypassRoutes_.find(route_name) == bypassRoutes_.end(), "The bypass route between %s@%s and %s@%s already exists.",
          src, e_route->gw_src->name(), dst, e_route->gw_dst->name());
    } else {
      XBT_DEBUG("Load bypassRoute from %s to %s", src, dst);
      xbt_assert(!e_route->link_list->empty(),                          "Bypass route between %s and %s cannot be empty.",    src, dst);
      xbt_assert(bypassRoutes_.find(route_name) == bypassRoutes_.end(), "The bypass route between %s and %s already exists.", src, dst);
    }

    /* Build a copy that will be stored in the dict */
    std::vector<Link*> *newRoute = new std::vector<Link*>();
    for (auto link: *e_route->link_list)
      newRoute->push_back(link);

    /* Store it */
    bypassRoutes_.insert({route_name, newRoute});
    xbt_free(route_name);
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
  simgrid::surf::NetCard *netcard = h==NULL ? NULL: h->pimpl_netcard;
  if (!netcard)
    netcard = (simgrid::surf::NetCard*) xbt_lib_get_or_null(as_router_lib, name, ROUTING_ASR_LEVEL);
  return netcard;
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

  xbt_assert(nullptr == xbt_lib_get_or_null(as_router_lib, AS->id, ROUTING_ASR_LEVEL),
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

  if (current_routing == NULL && routing_platf->root_ == NULL) {
    /* it is the first one */
    new_as->father_ = NULL;
    routing_platf->root_ = new_as;
    netcard->setId(-1);
  } else if (current_routing != NULL && routing_platf->root_ != NULL) {

    xbt_assert(!xbt_dict_get_or_null(current_routing->sons_, AS->id),
               "The AS \"%s\" already exists", AS->id);
    /* it is a part of the tree */
    new_as->father_ = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy_ == SURF_ROUTING_NULL)
      current_routing->hierarchy_ = SURF_ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->sons_, AS->id, (void *) new_as, NULL);
    /* add to the father element list */
    netcard->setId(current_routing->addComponent(netcard));
  } else {
    THROWF(arg_error, 0, "All defined components must belong to a AS");
  }

  xbt_lib_set(as_router_lib, netcard->name(), ROUTING_ASR_LEVEL, (void *) netcard);
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
 */
void routing_AS_end()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->Seal();
  current_routing = current_routing->father_;
}

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
    AS_t * res_father, AS_t * res_src, AS_t * res_dst)
{
  xbt_assert(src && dst, "bad parameters for \"elements_father\" method");
#define ROUTING_HIERARCHY_MAXDEPTH 16     /* increase if it is not enough */
  simgrid::surf::As *path_src[ROUTING_HIERARCHY_MAXDEPTH];
  simgrid::surf::As *path_dst[ROUTING_HIERARCHY_MAXDEPTH];
  int index_src = 0;
  int index_dst = 0;
  simgrid::surf::As *current_src;
  simgrid::surf::As *current_dst;
  simgrid::surf::As *father;

  /* (1) find the path to root of src and dst*/
  simgrid::surf::As *src_as = src->containingAS();
  simgrid::surf::As *dst_as = dst->containingAS();

  xbt_assert(src_as, "Host %s must be in an AS", src->name());
  xbt_assert(dst_as, "Host %s must be in an AS", dst->name());

  /* (2) find the path to the root routing component */
  for (simgrid::surf::As *current = src_as; current != NULL; current = current->father_) {
    if (index_src >= ROUTING_HIERARCHY_MAXDEPTH)
      xbt_die("ROUTING_HIERARCHY_MAXDEPTH should be increased for element %s", src->name());
    path_src[index_src++] = current;
  }
  for (simgrid::surf::As *current = dst_as; current != NULL; current = current->father_) {
    if (index_dst >= ROUTING_HIERARCHY_MAXDEPTH)
      xbt_die("ROUTING_HIERARCHY_MAXDEPTH should be increased for path_dst");
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

#undef ROUTING_HIERARCHY_MAXDEPTH
}

/**
 * \brief Recursive function for get_route_and_latency
 *
 * \param src the source host name
 * \param dst the destination host name
 * \param *route the route where the links are stored. It is either NULL or a ready to use dynar
 * \param *latency the latency, if needed
 */
static void _get_route_and_latency(simgrid::surf::NetCard *src, simgrid::surf::NetCard *dst,
    std::vector<Link*> * links, double *latency)
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

  /* Check whether a direct bypass is defined. If so, use it and bail out */
  std::vector<Link*> *bypassed_route = common_father->getBypassRoute(src, dst);
  if (nullptr != bypassed_route) {
    for (Link *link : *bypassed_route) {
      links->push_back(link);
      if (latency)
        *latency += link->getLatency();
    }
    return;
  }

  /* If src and dst are in the same AS, life is good */
  if (src_father == dst_father) {       /* SURF_ROUTING_BASE */
    route.link_list = links;
    common_father->getRouteAndLatency(src, dst, &route, latency);
    return;
  }

  /* Not in the same AS, no bypass. We'll have to find our path between the ASes recursively*/

  route.link_list = new std::vector<Link*>();

  common_father->getRouteAndLatency(src_father->netcard_, dst_father->netcard_, &route, latency);
  xbt_assert((route.gw_src != NULL) && (route.gw_dst != NULL),
      "bad gateways for route from \"%s\" to \"%s\"", src->name(), dst->name());

  /* If source gateway is not our source, we have to recursively find our way up to this point */
  if (src != route.gw_src)
    _get_route_and_latency(src, route.gw_src, links, latency);
  for (auto link: *route.link_list)
    links->push_back(link);

  /* If dest gateway is not our destination, we have to recursively find our way from this point */
  if (route.gw_dst != dst)
    _get_route_and_latency(route.gw_dst, dst, links, latency);

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
void RoutingPlatf::getRouteAndLatency(NetCard *src, NetCard *dst, std::vector<Link*> * route, double *latency)
{
  XBT_DEBUG("getRouteAndLatency from %s to %s", src->name(), dst->name());

  _get_route_and_latency(src, dst, route, latency);
}

static xbt_dynar_t _recursiveGetOneLinkRoutes(As *rc)
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
    xbt_dynar_t onelink_child = _recursiveGetOneLinkRoutes(rc_child);
    if (onelink_child)
      xbt_dynar_merge(&ret,&onelink_child);
  }
  return ret;
}

xbt_dynar_t RoutingPlatf::getOneLinkRoutes(){
  return _recursiveGetOneLinkRoutes(root_);
}

}
}

/** @brief create the root AS */
void routing_model_create(Link *loopback)
{
  routing_platf = new simgrid::surf::RoutingPlatf(loopback);
}

/* ************************************************************************** */
/* ************************* GENERIC PARSE FUNCTIONS ************************ */

void routing_cluster_add_backbone(simgrid::surf::Link* bb) {
  simgrid::surf::AsCluster *cluster = dynamic_cast<simgrid::surf::AsCluster*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(nullptr == cluster->backbone_, "Cluster %s already has a backbone link!", cluster->name_);

  cluster->backbone_ = bb;
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
    host.pstate        = 0;
    host.core_amount   = 1;

    s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
    memset(&link, 0, sizeof(link));
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
  host_id = bprintf("peer_%s", peer->id);
  link_id = bprintf("link_%s", peer->id);
  router_id = bprintf("router_%s", peer->id);

  XBT_DEBUG("<AS id=\"%s\"\trouting=\"Cluster\">", peer->id);
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  AS.id                    = peer->id;
  AS.routing               = A_surfxml_AS_routing_Cluster;
  sg_platf_new_AS_begin(&AS);

  XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\"/>", host_id, peer->speed);
  s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
  memset(&host, 0, sizeof(host));
  host.id = host_id;

  host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
  xbt_dynar_push(host.speed_peak,&peer->speed);
  host.pstate = 0;
  //host.power_peak = peer->power;
  host.speed_trace = peer->availability_trace;
  host.state_trace = peer->state_trace;
  host.core_amount = 1;
  sg_platf_new_host(&host);
  xbt_dynar_free(&host.speed_peak);

  s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
  memset(&link, 0, sizeof(link));
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
  static_cast<AsCluster*>(current_routing)->router_ = static_cast<NetCard*>(xbt_lib_get_or_null(as_router_lib, router.id, ROUTING_ASR_LEVEL));

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

/** \brief Frees all memory allocated by the routing module */
void routing_exit(void) {
  delete routing_platf;
}

namespace simgrid {
namespace surf {

  RoutingPlatf::RoutingPlatf(Link *loopback)
  : loopback_(loopback)
  {
  }
  RoutingPlatf::~RoutingPlatf()
  {
    delete root_;
  }

}
}

AS_t surf_AS_get_routing_root() {
  return routing_platf->root_;
}

const char *surf_AS_get_name(simgrid::surf::As *as) {
  return as->name_;
}

static simgrid::surf::As *surf_AS_recursive_get_by_name(simgrid::surf::As *current, const char * name)
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
  simgrid::surf::As *as = surf_AS_recursive_get_by_name(routing_platf->root_, name);
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
