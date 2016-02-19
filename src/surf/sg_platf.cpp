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
#include "simgrid/platf_interface.h"
#include "surf/surf.h"

#include "src/simix/smx_private.h"
#include "src/surf/platform.hpp"

#include "surf/surfxml_parse.h"// FIXME: brain dead public header

#include "src/surf/platform.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/host_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf_routing.h" // FIXME: brain dead public header
#include "src/surf/surf_routing_cluster.hpp"
#include "src/surf/surf_routing_cluster_torus.hpp"
#include "src/surf/surf_routing_cluster_fat_tree.hpp"

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
}

/** @brief Add an "host" to the current AS */
void sg_platf_new_host(sg_platf_host_cbarg_t host)
{
  xbt_assert(! sg_host_by_name(host->id),
      "Refusing to create a second host named '%s'.", host->id);

  simgrid::surf::As* current_routing = routing_get_current();
  if (current_routing->hierarchy_ == SURF_ROUTING_NULL)
    current_routing->hierarchy_ = SURF_ROUTING_BASE;

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
      host->pstate,
      host->speed_scale, host->speed_trace,
      host->core_amount,
      host->initiallyOn, host->state_trace);
  surf_host_model->createHost(host->id, netcard, cpu, host->properties)->attach(h);
  simgrid::s4u::Host::onCreation(*h);

  if (TRACE_is_enabled() && TRACE_needs_platform())
    sg_instr_new_host(host);
}

/**
 * \brief Add a "router" to the network element list
 */
void sg_platf_new_router(sg_platf_router_cbarg_t router)
{
  simgrid::surf::As* current_routing = routing_get_current();

  if (current_routing->hierarchy_ == SURF_ROUTING_NULL)
    current_routing->hierarchy_ = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(as_router_lib, router->id, ROUTING_ASR_LEVEL),
             "Reading a router, processing unit \"%s\" already exists",
             router->id);

  simgrid::surf::NetCard *info = new simgrid::surf::NetCardImpl(router->id, SURF_NETWORK_ELEMENT_ROUTER, current_routing);
  info->setId(current_routing->addComponent(info));
  xbt_lib_set(as_router_lib, router->id, ROUTING_ASR_LEVEL, (void *) info);
  XBT_DEBUG("Having set name '%s' id '%d'", router->id, info->id());
  simgrid::surf::netcardCreatedCallbacks(info);

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
  simgrid::surf::As *current_routing = routing_get_current();
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
        host.speed_trace = tmgr_trace_new_from_file(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tavailability_file=\"\"");
      }

      if (cluster->state_trace && strcmp(cluster->state_trace, "")) {
        char *avail_file = xbt_str_varsubst(cluster->state_trace, patterns);
        XBT_DEBUG("\tstate_file=\"%s\"", avail_file);
        host.state_trace = tmgr_trace_new_from_file(avail_file);
        xbt_free(avail_file);
      } else {
        XBT_DEBUG("\tstate_file=\"\"");
      }

      host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
      xbt_dynar_push(host.speed_peak,&cluster->speed);
      host.pstate = 0;

      //host.power_peak = cluster->power;
      host.speed_scale = 1.0;
      host.core_amount = cluster->core_amount;
      host.initiallyOn = 1;
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
        link.initiallyOn = 1;
        link.policy    = SURF_LINK_FATPIPE;
        sg_platf_new_link(&link);
        info_loop.link_up   = Link::byName(tmp_link);
        info_loop.link_down = info_loop.link_up;
        free(tmp_link);
        xbt_dynar_set(current_routing->upDownLinks,
          rankId*(static_cast<AsCluster*>(current_routing))->nb_links_per_node_, &info_loop);
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
        link.initiallyOn = 1;
        link.policy = SURF_LINK_SHARED;
        sg_platf_new_link(&link);
        info_lim.link_up = Link::byName(tmp_link);
        info_lim.link_down = info_lim.link_up;
        free(tmp_link);
        auto as_cluster = static_cast<AsCluster*>(current_routing);
        xbt_dynar_set(current_routing->upDownLinks,
            rankId*(as_cluster)->nb_links_per_node_ + as_cluster->has_loopback_ ,
            &info_lim);

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
    link.initiallyOn = 1;
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

  xbt_lib_set(storage_lib,
      storage->id,
      ROUTING_STORAGE_LEVEL,
      (void *) xbt_strdup(storage->type_id));

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
  routing_get_current()->parseRoute(route);
}

void sg_platf_new_bypassRoute(sg_platf_route_cbarg_t bypassRoute)
{
  routing_get_current()->parseBypassroute(bypassRoute);
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

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  simgrid::surf::on_postparse();
}

void sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS)
{
  if (!surf_parse_models_setup_already_called) {
    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|AS|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer create ASes internally, so putting the code in there is ok.
     *
     * TODO, There used to be a guard protecting here against
     * xbt_dynar_length(sg_platf_AS_begin_cb_list) because we don't want to
     * initialize the models if we are parsing the file to get the deployment.
     * That could happen if the same file would be used for platf and deploy:
     * it'd contain AS tags even during the deploy parsing. Removing that guard
     * would result of the models to get re-inited when parsing for deploy.
     * Currently using the same file for platform and deployment is broken
     * however. This guard will have to ba adapted in order to make this feature
     * work again.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  routing_AS_begin(AS);
  if (TRACE_is_enabled())
    sg_instr_AS_begin(AS);
}

void sg_platf_new_AS_end()
{
  routing_AS_end();
  if (TRACE_is_enabled())
    sg_instr_AS_end();
}
