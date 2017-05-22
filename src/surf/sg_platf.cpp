/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"

#include "src/kernel/EngineImpl.hpp"
#include "src/simix/smx_private.h"

#include "src/include/simgrid/sg_config.h"

#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"

#include "src/kernel/routing/ClusterZone.hpp"
#include "src/kernel/routing/DijkstraZone.hpp"
#include "src/kernel/routing/DragonflyZone.hpp"
#include "src/kernel/routing/EmptyZone.hpp"
#include "src/kernel/routing/FatTreeZone.hpp"
#include "src/kernel/routing/FloydZone.hpp"
#include "src/kernel/routing/FullZone.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"
#include "src/kernel/routing/TorusZone.hpp"
#include "src/kernel/routing/VivaldiZone.hpp"
#include <string>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

XBT_PRIVATE std::map<std::string, simgrid::surf::Storage*> mount_list;

namespace simgrid {
namespace surf {

simgrid::xbt::signal<void(sg_platf_cluster_cbarg_t)> on_cluster;

}
}

// FIXME: The following duplicates the content of s4u::Host
namespace simgrid {
namespace s4u {
extern std::map<std::string, simgrid::s4u::Host*> host_list;
}
}

static int surf_parse_models_setup_already_called = 0;
std::map<std::string, storage_type_t> storage_types;

/** The current AS in the parsing */
static simgrid::kernel::routing::NetZoneImpl* current_routing = nullptr;
static simgrid::kernel::routing::NetZoneImpl* routing_get_current()
{
  return current_routing;
}

/** Module management function: creates all internal data structures */
void sg_platf_init()
{ /* Do nothing: just for symmetry of user code */
}

/** Module management function: frees all internal data structures */
void sg_platf_exit() {
  simgrid::surf::on_cluster.disconnect_all_slots();
  simgrid::s4u::onPlatformCreated.disconnect_all_slots();

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
  surf_parse_lex_destroy();
}

/** @brief Add an host to the current AS */
void sg_platf_new_host(sg_platf_host_cbarg_t args)
{
  std::unordered_map<std::string, std::string> props;
  if (args->properties) {
    xbt_dict_cursor_t cursor=nullptr;
    char *key;
    char* data;
    xbt_dict_foreach (args->properties, cursor, key, data)
      props[key] = data;
    xbt_dict_free(&args->properties);
  }

  simgrid::s4u::Host* host =
      routing_get_current()->createHost(args->id, &args->speed_per_pstate, args->core_amount, &props);

  host->pimpl_->storage_ = mount_list;
  mount_list.clear();

  /* Change from the defaults */
  if (args->state_trace)
    host->pimpl_cpu->setStateTrace(args->state_trace);
  if (args->speed_trace)
    host->pimpl_cpu->setSpeedTrace(args->speed_trace);
  if (args->pstate != 0)
    host->pimpl_cpu->setPState(args->pstate);
  if (args->coord && strcmp(args->coord, ""))
    new simgrid::kernel::routing::vivaldi::Coords(host->pimpl_netpoint, args->coord);

  if (TRACE_is_enabled() && TRACE_needs_platform())
    sg_instr_new_host(*host);
}

/** @brief Add a "router" to the network element list */
simgrid::kernel::routing::NetPoint* sg_platf_new_router(const char* name, const char* coords)
{
  simgrid::kernel::routing::NetZoneImpl* current_routing = routing_get_current();

  if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
    current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::base;
  xbt_assert(nullptr == simgrid::s4u::Engine::instance()->netpointByNameOrNull(name),
             "Refusing to create a router named '%s': this name already describes a node.", name);

  simgrid::kernel::routing::NetPoint* netpoint =
      new simgrid::kernel::routing::NetPoint(name, simgrid::kernel::routing::NetPoint::Type::Router, current_routing);
  XBT_DEBUG("Router '%s' has the id %d", name, netpoint->id());

  if (coords && strcmp(coords, ""))
    new simgrid::kernel::routing::vivaldi::Coords(netpoint, coords);

  sg_instr_new_router(name);

  return netpoint;
}

void sg_platf_new_link(LinkCreationArgs* link)
{
  std::vector<std::string> names;

  if (link->policy == SURF_LINK_FULLDUPLEX) {
    names.push_back(link->id+ "_UP");
    names.push_back(link->id+ "_DOWN");
  } else {
    names.push_back(link->id);
  }
  for (auto link_name : names) {
    simgrid::surf::LinkImpl* l =
        surf_network_model->createLink(link_name.c_str(), link->bandwidth, link->latency, link->policy);

    if (link->properties) {
      xbt_dict_cursor_t cursor = nullptr;
      char* key;
      char* data;
      xbt_dict_foreach (link->properties, cursor, key, data)
        l->setProperty(key, data);
      xbt_dict_free(&link->properties);
    }

    if (link->latency_trace)
      l->setLatencyTrace(link->latency_trace);
    if (link->bandwidth_trace)
      l->setBandwidthTrace(link->bandwidth_trace);
    if (link->state_trace)
      l->setStateTrace(link->state_trace);
  }
}

void sg_platf_new_cluster(sg_platf_cluster_cbarg_t cluster)
{
  using simgrid::kernel::routing::ClusterZone;
  using simgrid::kernel::routing::DragonflyZone;
  using simgrid::kernel::routing::FatTreeZone;
  using simgrid::kernel::routing::TorusZone;

  int rankId=0;

  // What an inventive way of initializing the AS that I have as ancestor :-(
  s_sg_platf_AS_cbarg_t AS;
  AS.id = cluster->id;
  switch (cluster->topology) {
  case SURF_CLUSTER_TORUS:
    AS.routing = A_surfxml_AS_routing_ClusterTorus;
    break;
  case SURF_CLUSTER_DRAGONFLY:
    AS.routing = A_surfxml_AS_routing_ClusterDragonfly;
    break;
  case SURF_CLUSTER_FAT_TREE:
    AS.routing = A_surfxml_AS_routing_ClusterFatTree;
    break;
  default:
    AS.routing = A_surfxml_AS_routing_Cluster;
    break;
  }
  sg_platf_new_AS_begin(&AS);
  simgrid::kernel::routing::ClusterZone* current_as = static_cast<ClusterZone*>(routing_get_current());
  current_as->parse_specific_arguments(cluster);

  if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
    current_as->linkCountPerNode_++;
    current_as->hasLoopback_ = 1;
  }

  if(cluster->limiter_link > 0){
    current_as->linkCountPerNode_++;
    current_as->hasLimiter_ = 1;
  }

  for (int i : *cluster->radicals) {
    char * host_id = bprintf("%s%d%s", cluster->prefix, i, cluster->suffix);
    char * link_id = bprintf("%s_link_%d", cluster->id, i);

    XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id, cluster->speeds.front());

    s_sg_platf_host_cbarg_t host;
    memset(&host, 0, sizeof(host));
    host.id = host_id;
    if ((cluster->properties != nullptr) && (!xbt_dict_is_empty(cluster->properties))) {
      xbt_dict_cursor_t cursor=nullptr;
      char *key;
      char* data;
      host.properties = xbt_dict_new_homogeneous(free);

      xbt_dict_foreach(cluster->properties,cursor,key,data) {
        xbt_dict_set(host.properties, key, xbt_strdup(data), nullptr);
      }
    }

    host.speed_per_pstate = cluster->speeds;
    host.pstate = 0;
    host.core_amount = cluster->core_amount;
    host.coord = "";
    sg_platf_new_host(&host);
    XBT_DEBUG("</host>");

    XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id, cluster->bw, cluster->lat);

    // All links are saved in a matrix;
    // every row describes a single node; every node may have multiple links.
    // the first column may store a link from x to x if p_has_loopback is set
    // the second column may store a limiter link if p_has_limiter is set
    // other columns are to store one or more link for the node

    //add a loopback link
    simgrid::surf::LinkImpl* linkUp   = nullptr;
    simgrid::surf::LinkImpl* linkDown = nullptr;
    if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
      char *tmp_link = bprintf("%s_loopback", link_id);
      XBT_DEBUG("<loopback\tid=\"%s\"\tbw=\"%f\"/>", tmp_link, cluster->loopback_bw);

      LinkCreationArgs link;
      link.id        = tmp_link;
      link.bandwidth = cluster->loopback_bw;
      link.latency   = cluster->loopback_lat;
      link.policy    = SURF_LINK_FATPIPE;
      sg_platf_new_link(&link);
      linkUp   = simgrid::surf::LinkImpl::byName(tmp_link);
      linkDown = simgrid::surf::LinkImpl::byName(tmp_link);
      free(tmp_link);

      auto as_cluster = static_cast<ClusterZone*>(current_as);
      as_cluster->privateLinks_.insert({rankId * as_cluster->linkCountPerNode_, {linkUp, linkDown}});
    }

    //add a limiter link (shared link to account for maximal bandwidth of the node)
    linkUp   = nullptr;
    linkDown = nullptr;
    if(cluster->limiter_link > 0){
      char *tmp_link = bprintf("%s_limiter", link_id);
      XBT_DEBUG("<limiter\tid=\"%s\"\tbw=\"%f\"/>", tmp_link, cluster->limiter_link);

      LinkCreationArgs link;
      link.id = tmp_link;
      link.bandwidth = cluster->limiter_link;
      link.latency = 0;
      link.policy = SURF_LINK_SHARED;
      sg_platf_new_link(&link);
      linkDown = simgrid::surf::LinkImpl::byName(tmp_link);
      linkUp   = linkDown;
      free(tmp_link);
      current_as->privateLinks_.insert(
          {rankId * current_as->linkCountPerNode_ + current_as->hasLoopback_, {linkUp, linkDown}});
    }

    //call the cluster function that adds the others links
    if (cluster->topology == SURF_CLUSTER_FAT_TREE) {
      static_cast<FatTreeZone*>(current_as)->addProcessingNode(i);
    } else {
      current_as->create_links_for_node(cluster, i, rankId,
          rankId*current_as->linkCountPerNode_ + current_as->hasLoopback_ + current_as->hasLimiter_ );
    }
    xbt_free(link_id);
    xbt_free(host_id);
    rankId++;
  }
  xbt_dict_free(&cluster->properties);

  // Add a router.
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id);
  if (!cluster->router_id || !strcmp(cluster->router_id, "")) {
    char* newid         = bprintf("%s%s_router%s", cluster->prefix, cluster->id, cluster->suffix);
    current_as->router_ = sg_platf_new_router(newid, NULL);
    free(newid);
  } else {
    current_as->router_ = sg_platf_new_router(cluster->router_id, NULL);
  }

  //Make the backbone
  if ((cluster->bb_bw > 0) || (cluster->bb_lat > 0)) {

    LinkCreationArgs link;
    link.id        = std::string(cluster->id)+ "_backbone";
    link.bandwidth = cluster->bb_bw;
    link.latency   = cluster->bb_lat;
    link.policy    = cluster->bb_sharing_policy;

    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link.id.c_str(), cluster->bb_bw, cluster->bb_lat);
    sg_platf_new_link(&link);

    routing_cluster_add_backbone(simgrid::surf::LinkImpl::byName(link.id.c_str()));
  }

  XBT_DEBUG("</AS>");
  sg_platf_new_AS_seal();

  simgrid::surf::on_cluster(cluster);
  delete cluster->radicals;
}

void routing_cluster_add_backbone(simgrid::surf::LinkImpl* bb)
{
  simgrid::kernel::routing::ClusterZone* cluster =
      dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(nullptr == cluster->backbone_, "Cluster %s already has a backbone link!", cluster->name());

  cluster->backbone_ = bb;
  XBT_DEBUG("Add a backbone to AS '%s'", current_routing->name());
}

void sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet)
{
  for (int radical : *cabinet->radicals) {
    std::string hostname = std::string(cabinet->prefix) + std::to_string(radical) + std::string(cabinet->suffix);
    s_sg_platf_host_cbarg_t host;
    memset(&host, 0, sizeof(host));
    host.pstate           = 0;
    host.core_amount      = 1;
    host.id               = hostname.c_str();
    host.speed_per_pstate.push_back(cabinet->speed);
    sg_platf_new_host(&host);

    LinkCreationArgs link;
    link.policy    = SURF_LINK_FULLDUPLEX;
    link.latency   = cabinet->lat;
    link.bandwidth = cabinet->bw;
    link.id        = "link_" + hostname;
    sg_platf_new_link(&link);

    s_sg_platf_host_link_cbarg_t host_link;
    memset(&host_link, 0, sizeof(host_link));
    host_link.id        = hostname.c_str();
    host_link.link_up   = bprintf("link_%s_UP",hostname.c_str());
    host_link.link_down = bprintf("link_%s_DOWN",hostname.c_str());
    sg_platf_new_hostlink(&host_link);
    free((char*)host_link.link_up);
    free((char*)host_link.link_down);
  }
  delete cabinet->radicals;
}

void sg_platf_new_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Refusing to add a second storage named \"%s\"", storage->id);

  xbt_assert(storage_types.find(storage->type_id) != storage_types.end(), "No storage type '%s'", storage->type_id);
  storage_type_t stype = storage_types.at(storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'", storage->id, storage->type_id,
            storage->content);

  xbt_lib_set(storage_lib, storage->id, ROUTING_STORAGE_LEVEL, (void *) xbt_strdup(storage->type_id));

  // if storage content is not specified use the content of storage_type if any
  if (!strcmp(storage->content, "") && strcmp(stype->content, "")) {
    storage->content      = stype->content;
    storage->content_type = stype->content_type;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s) from storage type '%s' ", storage->id,
              stype->content_type, stype->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
            "\n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tcontent_type '%s' "
            "\n\t\tproperties '%p''\n",
            storage->id, stype->model, stype->type_id, storage->content, storage->content_type, storage->properties);

  auto s = surf_storage_model->createStorage(storage->id, stype->type_id, storage->content, storage->content_type,
                                             storage->attach);

  if (storage->properties) {
    xbt_dict_cursor_t cursor = nullptr;
    char *key;
    char* data;
    xbt_dict_foreach (storage->properties, cursor, key, data)
      s->setProperty(key, data);
    xbt_dict_free(&storage->properties);
  }
}

void sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type)
{
  xbt_assert(storage_types.find(storage_type->id) == storage_types.end(),
             "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->content_type = xbt_strdup(storage_type->content_type);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size;
  stype->model_properties = storage_type->model_properties;

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s', content '%s', and content_type '%s'",
            stype->type_id, stype->model, storage_type->content, storage_type->content_type);

  storage_types.insert({std::string(stype->type_id), stype});
}

void sg_platf_new_mount(sg_platf_mount_cbarg_t mount){
  xbt_assert(xbt_lib_get_or_null(storage_lib, mount->storageId, ROUTING_STORAGE_LEVEL),
      "Cannot mount non-existent disk \"%s\"", mount->storageId);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->storageId, mount->name);

  if (mount_list.empty())
    XBT_DEBUG("Create a Mount list for %s", A_surfxml_host_id);
  mount_list.insert(
      {std::string(mount->name), surf_storage_resource_priv(surf_storage_resource_by_name(mount->storageId))});
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
  sg_host_t host = sg_host_by_name(process->host);
  if (!host) {
    // The requested host does not exist. Do a nice message to the user
    std::string msg = std::string("Cannot create process '") + process->function + "': host '" + process->host +
                      "' does not exist\nExisting hosts: '";
    for (auto kv : simgrid::s4u::host_list) {
      simgrid::s4u::Host* host = kv.second;
      msg += host->name();
      msg += "', '";
      if (msg.length() > 1024) {
        msg.pop_back(); // remove trailing quote
        msg += "...(list truncated)......";
        break;
      }
    }
    xbt_die("%s", msg.c_str());
  }
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(process->function);
  xbt_assert(factory, "Function '%s' unknown", process->function);

  double start_time = process->start_time;
  double kill_time  = process->kill_time;
  int auto_restart = process->on_failure == SURF_ACTOR_ON_FAILURE_DIE ? 0 : 1;

  std::vector<std::string> args(process->argv, process->argv + process->argc);
  std::function<void()> code = factory(std::move(args));

  smx_process_arg_t arg = nullptr;

  arg = new simgrid::simix::ProcessArg();
  arg->name = std::string(process->argv[0]);
  arg->code = code;
  arg->data = nullptr;
  arg->host = host;
  arg->kill_time = kill_time;
  arg->properties = current_property_set;

  host->extension<simgrid::simix::Host>()->boot_processes.push_back(arg);

  if (start_time > SIMIX_get_clock()) {

    arg = new simgrid::simix::ProcessArg();
    arg->name = std::string(process->argv[0]);
    arg->code = std::move(code);
    arg->data = nullptr;
    arg->host = host;
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    XBT_DEBUG("Process %s@%s will be started at time %f", arg->name.c_str(), arg->host->cname(), start_time);
    SIMIX_timer_set(start_time, [arg, auto_restart]() {
      smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), std::move(arg->code), arg->data,
                                                                arg->host, arg->properties, nullptr);
      if (arg->kill_time >= 0)
        simcall_process_set_kill_time(actor, arg->kill_time);
      if (auto_restart)
        SIMIX_process_auto_restart_set(actor, auto_restart);
      delete arg;
    });
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", arg->name.c_str(), host->cname());

    smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), std::move(code), nullptr, host,
                                                              current_property_set, nullptr);

    /* The actor creation will fail if the host is currently dead, but that's fine */
    if (actor != nullptr) {
      if (arg->kill_time >= 0)
        simcall_process_set_kill_time(actor, arg->kill_time);
      if (auto_restart)
        SIMIX_process_auto_restart_set(actor, auto_restart);
    }
  }
  current_property_set = nullptr;
}

void sg_platf_new_peer(sg_platf_peer_cbarg_t peer)
{
  simgrid::kernel::routing::VivaldiZone* as = dynamic_cast<simgrid::kernel::routing::VivaldiZone*>(current_routing);
  xbt_assert(as, "<peer> tag can only be used in Vivaldi netzones.");

  std::vector<double> speedPerPstate;
  speedPerPstate.push_back(peer->speed);
  simgrid::s4u::Host* host = as->createHost(peer->id, &speedPerPstate, 1, nullptr);

  as->setPeerLink(host->pimpl_netpoint, peer->bw_in, peer->bw_out, peer->coord);

  /* Change from the defaults */
  if (peer->state_trace)
    host->pimpl_cpu->setStateTrace(peer->state_trace);
  if (peer->speed_trace)
    host->pimpl_cpu->setSpeedTrace(peer->speed_trace);
}

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  simgrid::s4u::onPlatformCreated();
}

/* Pick the right models for CPU, net and host, and call their model_init_preparse */
static void surf_config_models_setup()
{
  const char* host_model_name    = xbt_cfg_get_string("host/model");
  const char* network_model_name = xbt_cfg_get_string("network/model");
  const char* cpu_model_name     = xbt_cfg_get_string("cpu/model");
  const char* storage_model_name = xbt_cfg_get_string("storage/model");

  /* The compound host model is needed when using non-default net/cpu models */
  if ((!xbt_cfg_is_default_value("network/model") || !xbt_cfg_is_default_value("cpu/model")) &&
      xbt_cfg_is_default_value("host/model")) {
    host_model_name = "compound";
    xbt_cfg_set_string("host/model", host_model_name);
  }

  XBT_DEBUG("host model: %s", host_model_name);
  if (!strcmp(host_model_name, "compound")) {
    xbt_assert(cpu_model_name, "Set a cpu model to use with the 'compound' host model");
    xbt_assert(network_model_name, "Set a network model to use with the 'compound' host model");

    int cpu_id = find_model_description(surf_cpu_model_description, cpu_model_name);
    surf_cpu_model_description[cpu_id].model_init_preparse();

    int network_id = find_model_description(surf_network_model_description, network_model_name);
    surf_network_model_description[network_id].model_init_preparse();
  }

  XBT_DEBUG("Call host_model_init");
  int host_id = find_model_description(surf_host_model_description, host_model_name);
  surf_host_model_description[host_id].model_init_preparse();

  XBT_DEBUG("Call vm_model_init");
  surf_vm_model_init_HL13();

  XBT_DEBUG("Call storage_model_init");
  int storage_id = find_model_description(surf_storage_model_description, storage_model_name);
  surf_storage_model_description[storage_id].model_init_preparse();
}

/**
 * \brief Add an AS to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host,
 * router or sub-AS) added after this call and before the corresponding call
 * to sg_platf_new_AS_seal() will be added to this AS.
 *
 * Once this function was called, the configuration concerning the used
 * models cannot be changed anymore.
 *
 * @param AS the parameters defining the AS to build.
 */
simgrid::s4u::NetZone* sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS)
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

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */

  /* search the routing model */
  simgrid::kernel::routing::NetZoneImpl* new_as = nullptr;
  switch(AS->routing){
    case A_surfxml_AS_routing_Cluster:
      new_as = new simgrid::kernel::routing::ClusterZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_ClusterDragonfly:
      new_as = new simgrid::kernel::routing::DragonflyZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_ClusterTorus:
      new_as = new simgrid::kernel::routing::TorusZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_ClusterFatTree:
      new_as = new simgrid::kernel::routing::FatTreeZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_Dijkstra:
      new_as = new simgrid::kernel::routing::DijkstraZone(current_routing, AS->id, 0);
      break;
    case A_surfxml_AS_routing_DijkstraCache:
      new_as = new simgrid::kernel::routing::DijkstraZone(current_routing, AS->id, 1);
      break;
    case A_surfxml_AS_routing_Floyd:
      new_as = new simgrid::kernel::routing::FloydZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_Full:
      new_as = new simgrid::kernel::routing::FullZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_None:
      new_as = new simgrid::kernel::routing::EmptyZone(current_routing, AS->id);
      break;
    case A_surfxml_AS_routing_Vivaldi:
      new_as = new simgrid::kernel::routing::VivaldiZone(current_routing, AS->id);
      break;
    default:
      xbt_die("Not a valid model!");
      break;
  }

  if (current_routing == nullptr) { /* it is the first one */
    xbt_assert(simgrid::s4u::Engine::instance()->pimpl->netRoot_ == nullptr,
               "All defined components must belong to a AS");
    simgrid::s4u::Engine::instance()->pimpl->netRoot_ = new_as;

  } else {
    /* set the father behavior */
    if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
      current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::recursive;
    /* add to the sons dictionary */
    current_routing->children()->push_back(static_cast<simgrid::s4u::NetZone*>(new_as));
  }

  /* set the new current component of the tree */
  current_routing = new_as;

  if (TRACE_is_enabled())
    sg_instr_AS_begin(AS);

  return new_as;
}

/**
 * \brief Specify that the description of the current AS is finished
 *
 * Once you've declared all the content of your AS, you have to seal
 * it with this call. Your AS is not usable until you call this function.
 */
void sg_platf_new_AS_seal()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->seal();
  current_routing = static_cast<simgrid::kernel::routing::NetZoneImpl*>(current_routing->father());

  if (TRACE_is_enabled())
    sg_instr_AS_end();
}

/** @brief Add a link connecting an host to the rest of its AS (which must be cluster or vivaldi) */
void sg_platf_new_hostlink(sg_platf_host_link_cbarg_t hostlink)
{
  simgrid::kernel::routing::NetPoint* netpoint = sg_host_by_name(hostlink->id)->pimpl_netpoint;
  xbt_assert(netpoint, "Host '%s' not found!", hostlink->id);
  xbt_assert(dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing),
             "Only hosts from Cluster and Vivaldi ASes can get an host_link.");

  simgrid::surf::LinkImpl* linkUp   = simgrid::surf::LinkImpl::byName(hostlink->link_up);
  simgrid::surf::LinkImpl* linkDown = simgrid::surf::LinkImpl::byName(hostlink->link_down);

  xbt_assert(linkUp, "Link '%s' not found!", hostlink->link_up);
  xbt_assert(linkDown, "Link '%s' not found!", hostlink->link_down);

  auto as_cluster = static_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  if (as_cluster->privateLinks_.find(netpoint->id()) != as_cluster->privateLinks_.end())
    surf_parse_error("Host_link for '%s' is already defined!",hostlink->id);

  XBT_DEBUG("Push Host_link for host '%s' to position %d", netpoint->cname(), netpoint->id());
  as_cluster->privateLinks_.insert({netpoint->id(), {linkUp, linkDown}});
}

void sg_platf_new_trace(sg_platf_trace_cbarg_t trace)
{
  tmgr_trace_t tmgr_trace;
  if (trace->file && strcmp(trace->file, "") != 0) {
    tmgr_trace = tmgr_trace_new_from_file(trace->file);
  } else {
    xbt_assert(strcmp(trace->pc_data, ""),
        "Trace '%s' must have either a content, or point to a file on disk.",trace->id);
    tmgr_trace = tmgr_trace_new_from_string(trace->id, trace->pc_data, trace->periodicity);
  }
  xbt_dict_set(traces_set_list, trace->id, static_cast<void*>(tmgr_trace), nullptr);
}
