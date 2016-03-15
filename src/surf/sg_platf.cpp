/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "xbt/RngStream.h"
#include <xbt/signal.hpp>
#include "src/surf/HostImpl.hpp"
#include "surf/surf.h"

#include "src/simix/smx_private.h"

#include "src/include/simgrid/sg_config.h"
#include "src/surf/xml/platf_private.hpp"

#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf_routing.h" // FIXME: brain dead public header

#include "src/surf/AsImpl.hpp"
#include "src/surf/AsCluster.hpp"
#include "src/surf/AsClusterTorus.hpp"
#include "src/surf/AsClusterFatTree.hpp"
#include "src/surf/AsDijkstra.hpp"
#include "src/surf/AsFloyd.hpp"
#include "src/surf/AsFull.hpp"
#include "src/surf/AsNone.hpp"
#include "src/surf/AsVivaldi.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

XBT_PRIVATE xbt_dynar_t mount_list = NULL;

namespace simgrid {
namespace surf {

simgrid::xbt::signal<void(sg_platf_link_cbarg_t)> on_link;
simgrid::xbt::signal<void(sg_platf_cluster_cbarg_t)> on_cluster;
simgrid::xbt::signal<void(void)> on_postparse;

}
}

static int surf_parse_models_setup_already_called = 0;

/** The current AS in the parsing */
static simgrid::surf::AsImpl *current_routing = NULL;
static simgrid::surf::AsImpl* routing_get_current()
{
  return current_routing;
}

/** Module management function: creates all internal data structures */
void sg_platf_init(void) {
}

/** Module management function: frees all internal data structures */
void sg_platf_exit(void) {
  simgrid::surf::on_link.disconnect_all_slots();
  simgrid::surf::on_cluster.disconnect_all_slots();
  simgrid::surf::on_postparse.disconnect_all_slots();

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
  surf_parse_lex_destroy();
}

/** @brief Add an "host" to the current AS */
void sg_platf_new_host(sg_platf_host_cbarg_t host)
{
  xbt_assert(! sg_host_by_name(host->id),
      "Refusing to create a second host named '%s'.", host->id);

  simgrid::surf::AsImpl* current_routing = routing_get_current();
  if (current_routing->hierarchy_ == simgrid::surf::AsImpl::RoutingMode::unset)
    current_routing->hierarchy_ = simgrid::surf::AsImpl::RoutingMode::base;

  simgrid::surf::NetCard *netcard =
      new simgrid::surf::NetCardImpl(host->id, SURF_NETWORK_ELEMENT_HOST, current_routing);

  netcard->setId(current_routing->addComponent(netcard));
  sg_host_t h = simgrid::s4u::Host::by_name_or_create(host->id);
  h->pimpl_netcard = netcard;
  simgrid::surf::netcardCreatedCallbacks(netcard);

  if(mount_list){
    xbt_lib_set(storage_lib, host->id, ROUTING_STORAGE_HOST_LEVEL, (void *) mount_list);
    mount_list = NULL;
  }

  if (host->coord && strcmp(host->coord, "")) {
    unsigned int cursor;
    char*str;

    if (!COORD_HOST_LEVEL)
      xbt_die ("To use host coordinates, please add --cfg=network/coordinates:yes to your command line");
    /* Pre-parse the host coordinates -- FIXME factorize with routers by overloading the routing->parse_PU function*/
    xbt_dynar_t ctn_str = xbt_str_split_str(host->coord, " ");
    xbt_dynar_t ctn = xbt_dynar_new(sizeof(double),NULL);
    xbt_dynar_foreach(ctn_str,cursor, str) {
      double val = xbt_str_parse_double(str, "Invalid coordinate: %s");
      xbt_dynar_push(ctn,&val);
    }
    xbt_dynar_shrink(ctn, 0);
    xbt_dynar_free(&ctn_str);
    h->extension_set(COORD_HOST_LEVEL, (void *) ctn);
    XBT_DEBUG("Having set host coordinates for '%s'",host->id);
  }


  simgrid::surf::Cpu *cpu = surf_cpu_model_pm->createCpu( h,
      host->speed_peak,
      host->speed_trace,
      host->core_amount,
      host->state_trace);
  surf_host_model->createHost(host->id, netcard, cpu, host->properties)->attach(h);

  if (host->pstate != 0)
    cpu->setPState(host->pstate);

  simgrid::s4u::Host::onCreation(*h);

  if (TRACE_is_enabled() && TRACE_needs_platform())
    sg_instr_new_host(host);
}

/**
 * \brief Add a "router" to the network element list
 */
void sg_platf_new_router(sg_platf_router_cbarg_t router)
{
  simgrid::surf::AsImpl* current_routing = routing_get_current();

  if (current_routing->hierarchy_ == simgrid::surf::AsImpl::RoutingMode::unset)
    current_routing->hierarchy_ = simgrid::surf::AsImpl::RoutingMode::base;
  xbt_assert(nullptr == xbt_lib_get_or_null(as_router_lib, router->id, ROUTING_ASR_LEVEL),
             "Refusing to create a router named '%s': this name already describes a node.", router->id);

  simgrid::surf::NetCard *netcard = new simgrid::surf::NetCardImpl(router->id, SURF_NETWORK_ELEMENT_ROUTER, current_routing);
  netcard->setId(current_routing->addComponent(netcard));
  xbt_lib_set(as_router_lib, router->id, ROUTING_ASR_LEVEL, (void *) netcard);
  XBT_DEBUG("Having set name '%s' id '%d'", router->id, netcard->id());
  simgrid::surf::netcardCreatedCallbacks(netcard);

  if (router->coord && strcmp(router->coord, "")) {
    unsigned int cursor;
    char*str;

    if (!COORD_ASR_LEVEL)
      xbt_die ("To use host coordinates, please add --cfg=network/coordinates:yes to your command line");
    /* Pre-parse the host coordinates */
    xbt_dynar_t ctn_str = xbt_str_split_str(router->coord, " ");
    xbt_dynar_t ctn = xbt_dynar_new(sizeof(double),NULL);
    xbt_dynar_foreach(ctn_str,cursor, str) {
      double val = xbt_str_parse_double(str, "Invalid coordinate: %s");
      xbt_dynar_push(ctn,&val);
    }
    xbt_dynar_shrink(ctn, 0);
    xbt_dynar_free(&ctn_str);
    xbt_lib_set(as_router_lib, router->id, COORD_ASR_LEVEL, (void *) ctn);
    XBT_DEBUG("Having set router coordinates for '%s'",router->id);
  }

  if (TRACE_is_enabled() && TRACE_needs_platform())
    sg_instr_new_router(router);
}

void sg_platf_new_link(sg_platf_link_cbarg_t link){
  simgrid::surf::on_link(link);
}

void sg_platf_new_cluster(sg_platf_cluster_cbarg_t cluster)
{
  using simgrid::surf::AsCluster;
  using simgrid::surf::AsClusterTorus;
  using simgrid::surf::AsClusterFatTree;

  char *host_id, *groups, *link_id = NULL;
  xbt_dict_t patterns = NULL;
  int rankId=0;

  s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
  s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;

  unsigned int iter;

  if ((cluster->availability_trace && strcmp(cluster->availability_trace, ""))
      || (cluster->state_trace && strcmp(cluster->state_trace, ""))) {
    patterns = xbt_dict_new_homogeneous(xbt_free_f);
    xbt_dict_set(patterns, "id", xbt_strdup(cluster->id), NULL);
    xbt_dict_set(patterns, "prefix", xbt_strdup(cluster->prefix), NULL);
    xbt_dict_set(patterns, "suffix", xbt_strdup(cluster->suffix), NULL);
  }

  /* Parse the topology attributes.
   * Nothing to do in a vanilla cluster, but that's another story for torus and flat_trees */
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  AS.id = cluster->id;

  switch (cluster->topology) {
  case SURF_CLUSTER_TORUS:
    AS.routing = A_surfxml_AS_routing_ClusterTorus;
    break;
  case SURF_CLUSTER_FAT_TREE:
    AS.routing = A_surfxml_AS_routing_ClusterFatTree;
    break;
  default:
    AS.routing = A_surfxml_AS_routing_Cluster;
    break;
  }

  // What an inventive way of initializing the AS that I have as ancestor :-(
  sg_platf_new_AS_begin(&AS);
  simgrid::surf::AsImpl *current_routing = routing_get_current();
  static_cast<AsCluster*>(current_routing)->parse_specific_arguments(cluster);

  if(cluster->loopback_bw!=0 || cluster->loopback_lat!=0){
      ((AsCluster*)current_routing)->nb_links_per_node_++;
      ((AsCluster*)current_routing)->has_loopback_=1;
  }

  if(cluster->limiter_link!=0){
      ((AsCluster*)current_routing)->nb_links_per_node_++;
      ((AsCluster*)current_routing)->has_limiter_=1;
  }


  //Make all hosts
  xbt_dynar_t radical_elements = xbt_str_split(cluster->radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {

    xbt_dynar_t radical_ends = xbt_str_split(groups, "-");
    int start = surf_parse_get_int(xbt_dynar_get_as(radical_ends, 0, char *));
    int end;

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
    for (int i = start; i <= end; i++) {
      host_id = bprintf("%s%d%s", cluster->prefix, i, cluster->suffix);
      link_id = bprintf("%s_link_%d", cluster->id, i);

      XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id, cluster->speed);

      memset(&host, 0, sizeof(host));
      host.id = host_id;
      if ((cluster->properties != NULL) && (!xbt_dict_is_empty(cluster->properties))) {
        xbt_dict_cursor_t cursor=NULL;
        char *key,*data;
        host.properties = xbt_dict_new();

        xbt_dict_foreach(cluster->properties,cursor,key,data) {
          xbt_dict_set(host.properties, key, xbt_strdup(data),free);
        }
      }
      if (cluster->availability_trace && strcmp(cluster->availability_trace, "")) {
        xbt_dict_set(patterns, "radical", bprintf("%d", i), NULL);
        char *avail_file = xbt_str_varsubst(cluster->availability_trace, patterns);
        XBT_DEBUG("\tavailability_file=\"%s\"", avail_file);
        if (avail_file && avail_file[0])
          host.speed_trace = tmgr_trace_new_from_file(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tavailability_file=\"\"");
      }

      if (cluster->state_trace && strcmp(cluster->state_trace, "")) {
        char *avail_file = xbt_str_varsubst(cluster->state_trace, patterns);
        XBT_DEBUG("\tstate_file=\"%s\"", avail_file);
        if (avail_file && avail_file[0])
          host.state_trace = tmgr_trace_new_from_file(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tstate_file=\"\"");
      }

      host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
      xbt_dynar_push(host.speed_peak,&cluster->speed);
      host.pstate = 0;

      //host.power_peak = cluster->power;
      host.core_amount = cluster->core_amount;
      host.coord = "";
      sg_platf_new_host(&host);
      xbt_dynar_free(&host.speed_peak);
      XBT_DEBUG("</host>");

      XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id,
                cluster->bw, cluster->lat);


      s_surf_parsing_link_up_down_t info_lim, info_loop;
      // All links are saved in a matrix;
      // every row describes a single node; every node
      // may have multiple links.
      // the first column may store a link from x to x if p_has_loopback is set
      // the second column may store a limiter link if p_has_limiter is set
      // other columns are to store one or more link for the node

      //add a loopback link
      if(cluster->loopback_bw!=0 || cluster->loopback_lat!=0){
        char *tmp_link = bprintf("%s_loopback", link_id);
        XBT_DEBUG("<loopback\tid=\"%s\"\tbw=\"%f\"/>", tmp_link,
                cluster->limiter_link);


        memset(&link, 0, sizeof(link));
        link.id        = tmp_link;
        link.bandwidth = cluster->loopback_bw;
        link.latency   = cluster->loopback_lat;
        link.policy    = SURF_LINK_FATPIPE;
        sg_platf_new_link(&link);
        info_loop.link_up   = Link::byName(tmp_link);
        info_loop.link_down = info_loop.link_up;
        free(tmp_link);
        xbt_dynar_set(current_routing->upDownLinks, rankId*(static_cast<AsCluster*>(current_routing))->nb_links_per_node_, &info_loop);
      }

      //add a limiter link (shared link to account for maximal bandwidth of the node)
      if(cluster->limiter_link!=0){
        char *tmp_link = bprintf("%s_limiter", link_id);
        XBT_DEBUG("<limiter\tid=\"%s\"\tbw=\"%f\"/>", tmp_link,
                cluster->limiter_link);


        memset(&link, 0, sizeof(link));
        link.id = tmp_link;
        link.bandwidth = cluster->limiter_link;
        link.latency = 0;
        link.policy = SURF_LINK_SHARED;
        sg_platf_new_link(&link);
        info_lim.link_up = Link::byName(tmp_link);
        info_lim.link_down = info_lim.link_up;
        free(tmp_link);
        auto as_cluster = static_cast<AsCluster*>(current_routing);
        xbt_dynar_set(current_routing->upDownLinks, rankId*(as_cluster)->nb_links_per_node_ + as_cluster->has_loopback_ , &info_lim);

      }


      //call the cluster function that adds the others links
      if (cluster->topology == SURF_CLUSTER_FAT_TREE) {
        ((AsClusterFatTree*) current_routing)->addProcessingNode(i);
      }
      else {
      static_cast<AsCluster*>(current_routing)->create_links_for_node(cluster, i, rankId, rankId*
          static_cast<AsCluster*>(current_routing)->nb_links_per_node_
          + static_cast<AsCluster*>(current_routing)->has_loopback_
          + static_cast<AsCluster*>(current_routing)->has_limiter_ );
      }
      xbt_free(link_id);
      xbt_free(host_id);
      rankId++;
    }

    xbt_dynar_free(&radical_ends);
  }
  xbt_dynar_free(&radical_elements);

  // For fat trees, the links must be created once all nodes have been added
  if(cluster->topology == SURF_CLUSTER_FAT_TREE) {
    static_cast<simgrid::surf::AsClusterFatTree*>(current_routing)->create_links();
  }
  // Add a router. It is magically used thanks to the way in which surf_routing_cluster is written,
  // and it's very useful to connect clusters together
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id);
  char *newid = NULL;
  s_sg_platf_router_cbarg_t router = SG_PLATF_ROUTER_INITIALIZER;
  memset(&router, 0, sizeof(router));
  router.id = cluster->router_id;
  router.coord = "";
  if (!router.id || !strcmp(router.id, ""))
    router.id = newid =
        bprintf("%s%s_router%s", cluster->prefix, cluster->id,
                cluster->suffix);
  sg_platf_new_router(&router);
  ((AsCluster*)current_routing)->router_ = (simgrid::surf::NetCard*) xbt_lib_get_or_null(as_router_lib, router.id, ROUTING_ASR_LEVEL);
  free(newid);

  //Make the backbone
  if ((cluster->bb_bw != 0) || (cluster->bb_lat != 0)) {
    char *link_backbone = bprintf("%s_backbone", cluster->id);
    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link_backbone,
              cluster->bb_bw, cluster->bb_lat);

    memset(&link, 0, sizeof(link));
    link.id        = link_backbone;
    link.bandwidth = cluster->bb_bw;
    link.latency   = cluster->bb_lat;
    link.policy    = cluster->bb_sharing_policy;

    sg_platf_new_link(&link);

    routing_cluster_add_backbone(Link::byName(link_backbone));

    free(link_backbone);
  }

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_end();
  XBT_DEBUG(" ");
  xbt_dict_free(&patterns); // no op if it were never set

  simgrid::surf::on_cluster(cluster);
}
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

void sg_platf_new_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Refusing to add a second storage named \"%s\"", storage->id);

  void* stype = xbt_lib_get_or_null(storage_type_lib, storage->type_id,ROUTING_STORAGE_TYPE_LEVEL);
  xbt_assert(stype,"No storage type '%s'", storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'",
      storage->id,
      storage->type_id,
      storage->content);

  xbt_lib_set(storage_lib, storage->id, ROUTING_STORAGE_LEVEL, (void *) xbt_strdup(storage->type_id));

  // if storage content is not specified use the content of storage_type if any
  if(!strcmp(storage->content,"") && strcmp(((storage_type_t) stype)->content,"")){
    storage->content = ((storage_type_t) stype)->content;
    storage->content_type = ((storage_type_t) stype)->content_type;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s) from storage type '%s' ",
        storage->id,((storage_type_t) stype)->content_type,
        ((storage_type_t) stype)->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
      "\n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tcontent_type '%s' "
      "\n\t\tproperties '%p''\n",
      storage->id,
      ((storage_type_t) stype)->model,
      ((storage_type_t) stype)->type_id,
      storage->content,
      storage->content_type,
    storage->properties);

  surf_storage_model->createStorage(storage->id,
                                     ((storage_type_t) stype)->type_id,
                                     storage->content,
                                     storage->content_type,
                   storage->properties,
                                     storage->attach);
}
void sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type){

  xbt_assert(!xbt_lib_get_or_null(storage_type_lib, storage_type->id,ROUTING_STORAGE_TYPE_LEVEL),
               "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->content_type = xbt_strdup(storage_type->content_type);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size;
  stype->model_properties = storage_type->model_properties;

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s', "
      "content '%s', and content_type '%s'",
      stype->type_id,
      stype->model,
      storage_type->content,
      storage_type->content_type);

  xbt_lib_set(storage_type_lib,
      stype->type_id,
      ROUTING_STORAGE_TYPE_LEVEL,
      (void *) stype);
}
void sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage)
{
  THROW_UNIMPLEMENTED;
}

static void mount_free(void *p)
{
  mount_t mnt = (mount_t) p;
  xbt_free(mnt->name);
}

void sg_platf_new_mount(sg_platf_mount_cbarg_t mount){
  // Verification of an existing storage
  xbt_assert(xbt_lib_get_or_null(storage_lib, mount->storageId, ROUTING_STORAGE_LEVEL),
      "Cannot mount non-existent disk \"%s\"", mount->storageId);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->storageId, mount->name);

  s_mount_t mnt;
  mnt.storage = surf_storage_resource_priv(surf_storage_resource_by_name(mount->storageId));
  mnt.name = xbt_strdup(mount->name);

  if(!mount_list){
    XBT_DEBUG("Create a Mount list for %s",A_surfxml_host_id);
    mount_list = xbt_dynar_new(sizeof(s_mount_t), mount_free);
  }
  xbt_dynar_push(mount_list, &mnt);
}

void sg_platf_new_route(sg_platf_route_cbarg_t route)
{
  routing_get_current()->addRoute(route);
}

void sg_platf_new_bypassRoute(sg_platf_route_cbarg_t bypassRoute)
{
  routing_get_current()->addBypassRoute(bypassRoute);
}

void sg_platf_new_process(sg_platf_process_cbarg_t process)
{
  xbt_assert(simix_global,"Cannot create process without SIMIX.");

  sg_host_t host = sg_host_by_name(process->host);
  if (!host) {
    // The requested host does not exist. Do a nice message to the user
    char *tmp = bprintf("Cannot create process '%s': host '%s' does not exist\nExisting hosts: '",process->function, process->host);
    xbt_strbuff_t msg = xbt_strbuff_new_from(tmp);
    free(tmp);
    xbt_dynar_t all_hosts = xbt_dynar_sort_strings(sg_hosts_as_dynar());
    simgrid::s4u::Host* host;
    unsigned int cursor;
    xbt_dynar_foreach(all_hosts,cursor, host) {
      xbt_strbuff_append(msg,host->name().c_str());
      xbt_strbuff_append(msg,"', '");
      if (msg->used > 1024) {
        msg->data[msg->used-3]='\0';
        msg->used -= 3;

        xbt_strbuff_append(msg," ...(list truncated)......");// That will be shortened by 3 chars when existing the loop
      }
    }
    msg->data[msg->used-3]='\0';
    xbt_die("%s", msg->data);
  }
  xbt_main_func_t parse_code = SIMIX_get_registered_function(process->function);
  xbt_assert(parse_code, "Function '%s' unknown", process->function);

  double start_time = process->start_time;
  double kill_time  = process->kill_time;
  int auto_restart = process->on_failure == SURF_PROCESS_ON_FAILURE_DIE ? 0 : 1;

  smx_process_arg_t arg = NULL;
  smx_process_t process_created = NULL;

  arg = xbt_new0(s_smx_process_arg_t, 1);
  arg->code = parse_code;
  arg->data = NULL;
  arg->hostname = sg_host_get_name(host);
  arg->argc = process->argc;
  arg->argv = xbt_new(char *,process->argc);
  int i;
  for (i=0; i<process->argc; i++)
    arg->argv[i] = xbt_strdup(process->argv[i]);
  arg->name = xbt_strdup(arg->argv[0]);
  arg->kill_time = kill_time;
  arg->properties = current_property_set;
  if (!sg_host_simix(host)->boot_processes) {
    sg_host_simix(host)->boot_processes = xbt_dynar_new(sizeof(smx_process_arg_t), _SIMIX_host_free_process_arg);
  }
  xbt_dynar_push_as(sg_host_simix(host)->boot_processes,smx_process_arg_t,arg);

  if (start_time > SIMIX_get_clock()) {
    arg = xbt_new0(s_smx_process_arg_t, 1);
    arg->name = (char*)(process->argv)[0];
    arg->code = parse_code;
    arg->data = NULL;
    arg->hostname = sg_host_get_name(host);
    arg->argc = process->argc;
    arg->argv = (char**)(process->argv);
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    XBT_DEBUG("Process %s(%s) will be started at time %f", arg->name,
           arg->hostname, start_time);
    SIMIX_timer_set(start_time, [](void* arg) {
      SIMIX_process_create_from_wrapper((smx_process_arg_t) arg);
    }, arg);
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", arg->name, sg_host_get_name(host));

    if (simix_global->create_process_function)
      process_created = simix_global->create_process_function(
          arg->name,
                                            parse_code,
                                            NULL,
                                            sg_host_get_name(host),
                                            kill_time,
                                            process->argc,
                                            (char**)(process->argv),
                                            current_property_set,
                                            auto_restart, NULL);
    else
      process_created = simcall_process_create(arg->name, parse_code, NULL, sg_host_get_name(host), kill_time, process->argc,
          (char**)process->argv, current_property_set,auto_restart);

    /* verify if process has been created (won't be the case if the host is currently dead, but that's fine) */
    if (!process_created) {
      return;
    }
  }
  current_property_set = NULL;
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
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_up, peer->bw_out, peer->lat);
  link.id = link_up;
  link.bandwidth = peer->bw_out;
  sg_platf_new_link(&link);

  char* link_down = bprintf("%s_DOWN",link_id);
  XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_down, peer->bw_in, peer->lat);
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

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  simgrid::surf::on_postparse();
}

/* Pick the right models for CPU, net and host, and call their model_init_preparse */
static void surf_config_models_setup()
{
  const char *host_model_name;
  const char *vm_model_name;
  int host_id = -1;
  int vm_id = -1;
  char *network_model_name = NULL;
  char *cpu_model_name = NULL;
  int storage_id = -1;
  char *storage_model_name = NULL;

  host_model_name = xbt_cfg_get_string(_sg_cfg_set, "host/model");
  vm_model_name = xbt_cfg_get_string(_sg_cfg_set, "vm/model");
  network_model_name = xbt_cfg_get_string(_sg_cfg_set, "network/model");
  cpu_model_name = xbt_cfg_get_string(_sg_cfg_set, "cpu/model");
  storage_model_name = xbt_cfg_get_string(_sg_cfg_set, "storage/model");

  /* Check whether we use a net/cpu model differing from the default ones, in which case
   * we should switch to the "compound" host model to correctly dispatch stuff to
   * the right net/cpu models.
   */

  if ((!xbt_cfg_is_default_value(_sg_cfg_set, "network/model") ||
       !xbt_cfg_is_default_value(_sg_cfg_set, "cpu/model")) &&
      xbt_cfg_is_default_value(_sg_cfg_set, "host/model")) {
    host_model_name = "compound";
    xbt_cfg_set_string(_sg_cfg_set, "host/model", host_model_name);
  }

  XBT_DEBUG("host model: %s", host_model_name);
  host_id = find_model_description(surf_host_model_description, host_model_name);
  if (!strcmp(host_model_name, "compound")) {
    int network_id = -1;
    int cpu_id = -1;

    xbt_assert(cpu_model_name,
                "Set a cpu model to use with the 'compound' host model");

    xbt_assert(network_model_name,
                "Set a network model to use with the 'compound' host model");

    if(surf_cpu_model_init_preparse){
      surf_cpu_model_init_preparse();
    } else {
      cpu_id =
          find_model_description(surf_cpu_model_description, cpu_model_name);
      surf_cpu_model_description[cpu_id].model_init_preparse();
    }

    network_id =
        find_model_description(surf_network_model_description,
                               network_model_name);
    surf_network_model_description[network_id].model_init_preparse();
  }

  XBT_DEBUG("Call host_model_init");
  surf_host_model_description[host_id].model_init_preparse();

  XBT_DEBUG("Call vm_model_init");
  vm_id = find_model_description(surf_vm_model_description, vm_model_name);
  surf_vm_model_description[vm_id].model_init_preparse();

  XBT_DEBUG("Call storage_model_init");
  storage_id = find_model_description(surf_storage_model_description, storage_model_name);
  surf_storage_model_description[storage_id].model_init_preparse();

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
void sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS)
{
  if (!surf_parse_models_setup_already_called) {
    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|AS|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer create ASes internally, so putting the code in there is ok.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  xbt_assert(nullptr == xbt_lib_get_or_null(as_router_lib, AS->id, ROUTING_ASR_LEVEL),
      "Refusing to create a second AS called \"%s\".", AS->id);

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */


  /* search the routing model */
  simgrid::surf::AsImpl *new_as = NULL;
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
    if (current_routing->hierarchy_ == simgrid::surf::AsImpl::RoutingMode::unset)
      current_routing->hierarchy_ = simgrid::surf::AsImpl::RoutingMode::recursive;
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
  if (TRACE_is_enabled())
    sg_instr_AS_begin(AS);
}

/**
 * \brief Specify that the current description of AS is finished
 *
 * Once you've declared all the content of your AS, you have to close
 * it with this call. Your AS is not usable until you call this function.
 */
void sg_platf_new_AS_end()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->Seal();
  current_routing = static_cast<simgrid::surf::AsImpl*>(current_routing->father());

  if (TRACE_is_enabled())
    sg_instr_AS_end();
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
