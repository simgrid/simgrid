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
  simgrid::xbt::signal<void(simgrid::s4u::As*)> asCreatedCallbacks;


}} // namespace simgrid::surf

/**
 * @ingroup SURF_build_api
 * @brief A library containing all known hosts
 */
xbt_dict_t host_list = nullptr;

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
static simgrid::s4u::As *current_routing = NULL;
simgrid::s4u::As* routing_get_current()
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
  simgrid::s4u::As *new_as = NULL;
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
  simgrid::surf::NetCard *netcard = new simgrid::surf::NetCardImpl(new_as->name(), SURF_NETWORK_ELEMENT_AS, current_routing);

  if (current_routing == NULL && routing_platf->root_ == NULL) {
    /* it is the first one */
    routing_platf->root_ = new_as;
    netcard->setId(-1);
  } else if (current_routing != NULL && routing_platf->root_ != NULL) {

    xbt_assert(!xbt_dict_get_or_null(current_routing->children(), AS->id),
               "The AS \"%s\" already exists", AS->id);
    /* it is a part of the tree */
    new_as->father_ = current_routing;
    /* set the father behavior */
    if (current_routing->hierarchy_ == simgrid::s4u::As::ROUTING_NULL)
      current_routing->hierarchy_ = simgrid::s4u::As::ROUTING_RECURSIVE;
    /* add to the sons dictionary */
    xbt_dict_set(current_routing->children(), AS->id, (void *) new_as, NULL);
    /* add to the father element list */
    netcard->setId(current_routing->addComponent(netcard));
  } else {
    THROWF(arg_error, 0, "All defined components must belong to a AS");
  }

  xbt_lib_set(as_router_lib, netcard->name(), ROUTING_ASR_LEVEL, (void *) netcard);
  XBT_DEBUG("Having set name '%s' id '%d'", new_as->name(), netcard->id());

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
  current_routing = current_routing->father();
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

  s4u::As::getRouteRecursive(src, dst, route, latency);
}

static xbt_dynar_t _recursiveGetOneLinkRoutes(s4u::As *as)
{
  xbt_dynar_t ret = xbt_dynar_new(sizeof(Onelink*), xbt_free_f);

  //adding my one link routes
  xbt_dynar_t onelink_mine = as->getOneLinkRoutes();
  if (onelink_mine)
    xbt_dynar_merge(&ret,&onelink_mine);

  //recursing
  char *key;
  xbt_dict_cursor_t cursor = NULL;
  AS_t rc_child;
  xbt_dict_foreach(as->children(), cursor, key, rc_child) {
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
  xbt_assert(nullptr == cluster->backbone_, "Cluster %s already has a backbone link!", cluster->name());

  cluster->backbone_ = bb;
  XBT_DEBUG("Add a backbone to AS '%s'", current_routing->name());
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

simgrid::surf::RoutingPlatf::RoutingPlatf(simgrid::surf::Link *loopback)
: loopback_(loopback)
{
}
simgrid::surf::RoutingPlatf::~RoutingPlatf()
{
  delete root_;
}
