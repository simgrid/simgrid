/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/DijkstraZone.hpp"
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/kernel/routing/EmptyZone.hpp"
#include "simgrid/kernel/routing/FatTreeZone.hpp"
#include "simgrid/kernel/routing/FloydZone.hpp"
#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/kernel/routing/VivaldiZone.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/include/simgrid/sg_config.hpp"
#include "src/include/surf/surf.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <string>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

XBT_PRIVATE std::map<std::string, simgrid::kernel::resource::StorageImpl*> mount_list;
XBT_PRIVATE std::vector<std::string> known_storages;

namespace simgrid {
namespace kernel {
namespace routing {
xbt::signal<void(ClusterCreationArgs const&)> on_cluster_creation;
} // namespace routing
} // namespace kernel
} // namespace simgrid

static int surf_parse_models_setup_already_called = 0;
std::map<std::string, simgrid::kernel::resource::StorageType*> storage_types;

/** The current AS in the parsing */
static simgrid::kernel::routing::NetZoneImpl* current_routing = nullptr;
static simgrid::kernel::routing::NetZoneImpl* routing_get_current()
{
  return current_routing;
}

/** Module management function: creates all internal data structures */
void sg_platf_init()
{
  simgrid::s4u::Engine::on_platform_created.connect(check_disk_attachment);
}

/** Module management function: frees all internal data structures */
void sg_platf_exit() {
  simgrid::kernel::routing::on_cluster_creation.disconnect_slots();
  simgrid::s4u::Engine::on_platform_created.disconnect_slots();

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
  surf_parse_lex_destroy();
}

/** @brief Add a host to the current AS */
void sg_platf_new_host(const simgrid::kernel::routing::HostCreationArgs* args)
{
  std::map<std::string, std::string> props;
  if (args->properties) {
    for (auto const& elm : *args->properties)
      props.insert({elm.first, elm.second});
    delete args->properties;
  }

  simgrid::s4u::Host* host =
      routing_get_current()->create_host(args->id, args->speed_per_pstate, args->core_amount, &props);

  host->pimpl_->set_storages(mount_list);
  mount_list.clear();

  host->pimpl_->set_disks(args->disks, host);

  /* Change from the defaults */
  if (args->state_trace)
    host->set_state_profile(args->state_trace);
  if (args->speed_trace)
    host->set_speed_profile(args->speed_trace);
  if (args->pstate != 0)
    host->set_pstate(args->pstate);
  if (not args->coord.empty())
    new simgrid::kernel::routing::vivaldi::Coords(host->get_netpoint(), args->coord);
}

/** @brief Add a "router" to the network element list */
simgrid::kernel::routing::NetPoint* sg_platf_new_router(const std::string& name, const char* coords)
{
  if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
    current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::base;
  xbt_assert(nullptr == simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(name),
             "Refusing to create a router named '%s': this name already describes a node.", name.c_str());

  simgrid::kernel::routing::NetPoint* netpoint =
      new simgrid::kernel::routing::NetPoint(name, simgrid::kernel::routing::NetPoint::Type::Router, current_routing);
  XBT_DEBUG("Router '%s' has the id %u", netpoint->get_cname(), netpoint->id());

  if (coords && strcmp(coords, ""))
    new simgrid::kernel::routing::vivaldi::Coords(netpoint, coords);


  return netpoint;
}

static void sg_platf_new_link(const simgrid::kernel::routing::LinkCreationArgs* link, const std::string& link_name)
{
  static double last_warned_latency = sg_surf_precision;
  if (link->latency != 0.0 && link->latency < last_warned_latency) {
    XBT_WARN("Latency for link %s is smaller than surf/precision (%g < %g)."
             " For more accuracy, consider setting \"--cfg=surf/precision:%g\".",
             link_name.c_str(), link->latency, sg_surf_precision, link->latency);
    last_warned_latency = link->latency;
  }
  simgrid::kernel::resource::LinkImpl* l =
      surf_network_model->create_link(link_name, link->bandwidths, link->latency, link->policy);

  if (link->properties) {
    l->set_properties(*link->properties);
  }

  if (link->latency_trace)
    l->set_latency_profile(link->latency_trace);
  if (link->bandwidth_trace)
    l->set_bandwidth_profile(link->bandwidth_trace);
  if (link->state_trace)
    l->set_state_profile(link->state_trace);
}

void sg_platf_new_link(const simgrid::kernel::routing::LinkCreationArgs* link)
{
  if (link->policy == simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX) {
    sg_platf_new_link(link, link->id + "_UP");
    sg_platf_new_link(link, link->id + "_DOWN");
  } else {
    sg_platf_new_link(link, link->id);
  }
  delete link->properties;
}

void sg_platf_new_cluster(simgrid::kernel::routing::ClusterCreationArgs* cluster)
{
  using simgrid::kernel::routing::ClusterZone;
  using simgrid::kernel::routing::DragonflyZone;
  using simgrid::kernel::routing::FatTreeZone;
  using simgrid::kernel::routing::TorusZone;

  int rankId=0;

  // What an inventive way of initializing the AS that I have as ancestor :-(
  simgrid::kernel::routing::ZoneCreationArgs zone;
  zone.id = cluster->id;
  switch (cluster->topology) {
    case simgrid::kernel::routing::ClusterTopology::TORUS:
      zone.routing = A_surfxml_AS_routing_ClusterTorus;
      break;
    case simgrid::kernel::routing::ClusterTopology::DRAGONFLY:
      zone.routing = A_surfxml_AS_routing_ClusterDragonfly;
      break;
    case simgrid::kernel::routing::ClusterTopology::FAT_TREE:
      zone.routing = A_surfxml_AS_routing_ClusterFatTree;
      break;
    default:
      zone.routing = A_surfxml_AS_routing_Cluster;
      break;
  }
  sg_platf_new_Zone_begin(&zone);
  simgrid::kernel::routing::ClusterZone* current_as = static_cast<ClusterZone*>(routing_get_current());
  current_as->parse_specific_arguments(cluster);
  if (cluster->properties != nullptr)
    for (auto const& elm : *cluster->properties)
      current_as->get_iface()->set_property(elm.first, elm.second);

  if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
    current_as->num_links_per_node_++;
    current_as->has_loopback_ = true;
  }

  if(cluster->limiter_link > 0){
    current_as->num_links_per_node_++;
    current_as->has_limiter_ = true;
  }

  for (int const& i : *cluster->radicals) {
    std::string host_id = std::string(cluster->prefix) + std::to_string(i) + cluster->suffix;
    std::string link_id = std::string(cluster->id) + "_link_" + std::to_string(i);

    XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id.c_str(), cluster->speeds.front());

    simgrid::kernel::routing::HostCreationArgs host;
    host.id = host_id;
    if ((cluster->properties != nullptr) && (not cluster->properties->empty())) {
      host.properties = new std::unordered_map<std::string, std::string>();

      for (auto const& elm : *cluster->properties)
        host.properties->insert({elm.first, elm.second});
    }

    host.speed_per_pstate = cluster->speeds;
    host.pstate = 0;
    host.core_amount = cluster->core_amount;
    host.coord = "";
    sg_platf_new_host(&host);
    XBT_DEBUG("</host>");

    XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id.c_str(), cluster->bw, cluster->lat);

    // All links are saved in a matrix;
    // every row describes a single node; every node may have multiple links.
    // the first column may store a link from x to x if p_has_loopback is set
    // the second column may store a limiter link if p_has_limiter is set
    // other columns are to store one or more link for the node

    //add a loopback link
    const simgrid::s4u::Link* linkUp   = nullptr;
    const simgrid::s4u::Link* linkDown = nullptr;
    if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
      std::string tmp_link = link_id + "_loopback";
      XBT_DEBUG("<loopback\tid=\"%s\"\tbw=\"%f\"/>", tmp_link.c_str(), cluster->loopback_bw);

      simgrid::kernel::routing::LinkCreationArgs link;
      link.id        = tmp_link;
      link.bandwidths.push_back(cluster->loopback_bw);
      link.latency   = cluster->loopback_lat;
      link.policy    = simgrid::s4u::Link::SharingPolicy::FATPIPE;
      sg_platf_new_link(&link);
      linkUp   = simgrid::s4u::Link::by_name_or_null(tmp_link);
      linkDown = simgrid::s4u::Link::by_name_or_null(tmp_link);

      ClusterZone* as_cluster = current_as;
      as_cluster->private_links_.insert({as_cluster->node_pos(rankId), {linkUp->get_impl(), linkDown->get_impl()}});
    }

    //add a limiter link (shared link to account for maximal bandwidth of the node)
    linkUp   = nullptr;
    linkDown = nullptr;
    if(cluster->limiter_link > 0){
      std::string tmp_link = std::string(link_id) + "_limiter";
      XBT_DEBUG("<limiter\tid=\"%s\"\tbw=\"%f\"/>", tmp_link.c_str(), cluster->limiter_link);

      simgrid::kernel::routing::LinkCreationArgs link;
      link.id        = tmp_link;
      link.bandwidths.push_back(cluster->limiter_link);
      link.latency = 0;
      link.policy    = simgrid::s4u::Link::SharingPolicy::SHARED;
      sg_platf_new_link(&link);
      linkDown = simgrid::s4u::Link::by_name_or_null(tmp_link);
      linkUp   = linkDown;
      current_as->private_links_.insert(
          {current_as->node_pos_with_loopback(rankId), {linkUp->get_impl(), linkDown->get_impl()}});
    }

    //call the cluster function that adds the others links
    if (cluster->topology == simgrid::kernel::routing::ClusterTopology::FAT_TREE) {
      static_cast<FatTreeZone*>(current_as)->add_processing_node(i);
    } else {
      current_as->create_links_for_node(cluster, i, rankId, current_as->node_pos_with_loopback_limiter(rankId));
    }
    rankId++;
  }
  delete cluster->properties;

  // Add a router.
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id.c_str());
  if (cluster->router_id.empty())
    cluster->router_id = std::string(cluster->prefix) + cluster->id + "_router" + cluster->suffix;
  current_as->router_ = sg_platf_new_router(cluster->router_id, NULL);

  //Make the backbone
  if ((cluster->bb_bw > 0) || (cluster->bb_lat > 0)) {
    simgrid::kernel::routing::LinkCreationArgs link;
    link.id        = std::string(cluster->id)+ "_backbone";
    link.bandwidths.push_back(cluster->bb_bw);
    link.latency   = cluster->bb_lat;
    link.policy    = cluster->bb_sharing_policy;

    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link.id.c_str(), cluster->bb_bw, cluster->bb_lat);
    sg_platf_new_link(&link);

    routing_cluster_add_backbone(simgrid::s4u::Link::by_name(link.id)->get_impl());
  }

  XBT_DEBUG("</AS>");
  sg_platf_new_Zone_seal();

  simgrid::kernel::routing::on_cluster_creation(*cluster);
  delete cluster->radicals;
}

void routing_cluster_add_backbone(simgrid::kernel::resource::LinkImpl* bb)
{
  simgrid::kernel::routing::ClusterZone* cluster =
      dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(nullptr == cluster->backbone_, "Cluster %s already has a backbone link!", cluster->get_cname());

  cluster->backbone_ = bb;
  XBT_DEBUG("Add a backbone to AS '%s'", current_routing->get_cname());
}

void sg_platf_new_cabinet(const simgrid::kernel::routing::CabinetCreationArgs* cabinet)
{
  for (int const& radical : *cabinet->radicals) {
    std::string hostname = cabinet->prefix + std::to_string(radical) + cabinet->suffix;
    simgrid::kernel::routing::HostCreationArgs host;
    host.pstate           = 0;
    host.core_amount      = 1;
    host.id               = hostname;
    host.speed_per_pstate.push_back(cabinet->speed);
    sg_platf_new_host(&host);

    simgrid::kernel::routing::LinkCreationArgs link;
    link.policy    = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
    link.latency   = cabinet->lat;
    link.bandwidths.push_back(cabinet->bw);
    link.id        = "link_" + hostname;
    sg_platf_new_link(&link);

    simgrid::kernel::routing::HostLinkCreationArgs host_link;
    host_link.id        = hostname;
    host_link.link_up   = std::string("link_") + hostname + "_UP";
    host_link.link_down = std::string("link_") + hostname + "_DOWN";
    sg_platf_new_hostlink(&host_link);
  }
  delete cabinet->radicals;
}

simgrid::kernel::resource::DiskImpl* sg_platf_new_disk(const simgrid::kernel::routing::DiskCreationArgs* disk)
{
  simgrid::kernel::resource::DiskImpl* d = surf_disk_model->createDisk(disk->id, disk->read_bw, disk->write_bw);
  if (disk->properties) {
    d->set_properties(*disk->properties);
    delete disk->properties;
  }
  simgrid::s4u::Disk::on_creation(*d->get_iface());
  return d;
}

void sg_platf_new_storage(simgrid::kernel::routing::StorageCreationArgs* storage)
{
  xbt_assert(std::find(known_storages.begin(), known_storages.end(), storage->id) == known_storages.end(),
             "Refusing to add a second storage named \"%s\"", storage->id.c_str());

  const simgrid::kernel::resource::StorageType* stype;
  auto st = storage_types.find(storage->type_id);
  if (st != storage_types.end()) {
    stype = st->second;
  } else {
    xbt_die("No storage type '%s'", storage->type_id.c_str());
  }

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'", storage->id.c_str(),
            storage->type_id.c_str(), storage->content.c_str());

  known_storages.push_back(storage->id);

  // if storage content is not specified use the content of storage_type if any
  if (storage->content.empty() && not stype->content.empty()) {
    storage->content = stype->content;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s)", storage->id.c_str(),
              stype->id.c_str());
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
            "\n\t\tmodel '%s' \n\t\tcontent '%s' "
            "\n\t\tproperties '%p''\n",
            storage->id.c_str(), stype->model.c_str(), stype->id.c_str(), storage->content.c_str(),
            storage->properties);

  auto s = surf_storage_model->createStorage(storage->filename, storage->lineno, storage->id, stype->id,
                                             storage->content, storage->attach);

  if (storage->properties) {
    s->set_properties(*storage->properties);
    delete storage->properties;
  }
}

void sg_platf_new_storage_type(const simgrid::kernel::routing::StorageTypeCreationArgs* storage_type)
{
  xbt_assert(storage_types.find(storage_type->id) == storage_types.end(),
             "Reading a storage type, processing unit \"%s\" already exists", storage_type->id.c_str());

  simgrid::kernel::resource::StorageType* stype = new simgrid::kernel::resource::StorageType(
      storage_type->id, storage_type->model, storage_type->content, storage_type->properties,
      storage_type->model_properties, storage_type->size);

  XBT_DEBUG("Create a storage type id '%s' with model '%s', content '%s'", storage_type->id.c_str(),
            storage_type->model.c_str(), storage_type->content.c_str());

  storage_types[storage_type->id] = stype;
}

void sg_platf_new_mount(simgrid::kernel::routing::MountCreationArgs* mount)
{
  xbt_assert(std::find(known_storages.begin(), known_storages.end(), mount->storageId) != known_storages.end(),
             "Cannot mount non-existent disk \"%s\"", mount->storageId.c_str());

  XBT_DEBUG("Mount '%s' on '%s'", mount->storageId.c_str(), mount->name.c_str());

  if (mount_list.empty())
    XBT_DEBUG("Create a Mount list for %s", A_surfxml_host_id);
  mount_list.insert({mount->name, simgrid::s4u::Engine::get_instance()->storage_by_name(mount->storageId)->get_impl()});
}

void sg_platf_new_route(simgrid::kernel::routing::RouteCreationArgs* route)
{
  routing_get_current()->add_route(route->src, route->dst, route->gw_src, route->gw_dst, route->link_list,
                                   route->symmetrical);
}

void sg_platf_new_bypassRoute(simgrid::kernel::routing::RouteCreationArgs* bypassRoute)
{
  routing_get_current()->add_bypass_route(bypassRoute->src, bypassRoute->dst, bypassRoute->gw_src, bypassRoute->gw_dst,
                                          bypassRoute->link_list, bypassRoute->symmetrical);
}

void sg_platf_new_actor(simgrid::kernel::routing::ActorCreationArgs* actor)
{
  sg_host_t host = sg_host_by_name(actor->host);
  if (not host) {
    // The requested host does not exist. Do a nice message to the user
    std::string msg = std::string("Cannot create actor '") + actor->function + "': host '" + actor->host +
                      "' does not exist\nExisting hosts: '";

    std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::get_instance()->get_all_hosts();

    for (auto const& some_host : list) {
      msg += some_host->get_name();
      msg += "', '";
      if (msg.length() > 1024) {
        msg.pop_back(); // remove trailing quote
        msg += "...(list truncated)......";
        break;
      }
    }
    xbt_die("%s", msg.c_str());
  }
  const simgrid::kernel::actor::ActorCodeFactory& factory =
      simgrid::kernel::EngineImpl::get_instance()->get_function(actor->function);
  xbt_assert(factory, "Error while creating an actor from the XML file: Function '%s' not registered", actor->function);

  double start_time = actor->start_time;
  double kill_time  = actor->kill_time;
  bool auto_restart = actor->restart_on_failure;

  std::string actor_name     = actor->args[0];
  simgrid::kernel::actor::ActorCode code = factory(std::move(actor->args));
  std::shared_ptr<std::unordered_map<std::string, std::string>> properties(actor->properties);

  simgrid::kernel::actor::ProcessArg* arg =
      new simgrid::kernel::actor::ProcessArg(actor_name, code, nullptr, host, kill_time, properties, auto_restart);

  host->pimpl_->add_actor_at_boot(arg);

  if (start_time > SIMIX_get_clock()) {
    arg = new simgrid::kernel::actor::ProcessArg(actor_name, code, nullptr, host, kill_time, properties, auto_restart);

    XBT_DEBUG("Process %s@%s will be started at time %f", arg->name.c_str(), arg->host->get_cname(), start_time);
    simgrid::simix::Timer::set(start_time, [arg, auto_restart]() {
      simgrid::kernel::actor::ActorImplPtr new_actor = simgrid::kernel::actor::ActorImpl::create(
          arg->name.c_str(), std::move(arg->code), arg->data, arg->host, arg->properties.get(), nullptr);
      if (arg->kill_time >= 0)
        new_actor->set_kill_time(arg->kill_time);
      if (auto_restart)
        new_actor->set_auto_restart(auto_restart);
      delete arg;
    });
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", arg->name.c_str(), host->get_cname());

    try {
      simgrid::kernel::actor::ActorImplPtr new_actor = nullptr;
      new_actor = simgrid::kernel::actor::ActorImpl::create(arg->name.c_str(), std::move(code), nullptr, host,
                                                            arg->properties.get(), nullptr);
      /* The actor creation will fail if the host is currently dead, but that's fine */
      if (arg->kill_time >= 0)
        new_actor->set_kill_time(arg->kill_time);
      if (auto_restart)
        new_actor->set_auto_restart(auto_restart);
    } catch (simgrid::HostFailureException const&) {
      XBT_WARN("Deployment includes some initially turned off Hosts ... nevermind.");
    }
  }
}

void sg_platf_new_peer(const simgrid::kernel::routing::PeerCreationArgs* peer)
{
  simgrid::kernel::routing::VivaldiZone* as = dynamic_cast<simgrid::kernel::routing::VivaldiZone*>(current_routing);
  xbt_assert(as, "<peer> tag can only be used in Vivaldi netzones.");

  std::vector<double> speed_per_pstate;
  speed_per_pstate.push_back(peer->speed);
  simgrid::s4u::Host* host = as->create_host(peer->id, speed_per_pstate, 1, nullptr);

  as->set_peer_link(host->get_netpoint(), peer->bw_in, peer->bw_out, peer->coord);

  /* Change from the defaults */
  if (peer->state_trace)
    host->set_state_profile(peer->state_trace);
  if (peer->speed_trace)
    host->set_speed_profile(peer->speed_trace);
}

/* Pick the right models for CPU, net and host, and call their model_init_preparse */
static void surf_config_models_setup()
{
  std::string host_model_name    = simgrid::config::get_value<std::string>("host/model");
  std::string network_model_name = simgrid::config::get_value<std::string>("network/model");
  std::string cpu_model_name     = simgrid::config::get_value<std::string>("cpu/model");
  std::string disk_model_name    = simgrid::config::get_value<std::string>("disk/model");
  std::string storage_model_name = simgrid::config::get_value<std::string>("storage/model");

  /* The compound host model is needed when using non-default net/cpu models */
  if ((not simgrid::config::is_default("network/model") || not simgrid::config::is_default("cpu/model")) &&
      simgrid::config::is_default("host/model")) {
    host_model_name = "compound";
    simgrid::config::set_value("host/model", host_model_name);
  }

  XBT_DEBUG("host model: %s", host_model_name.c_str());
  if (host_model_name == "compound") {
    xbt_assert(not cpu_model_name.empty(), "Set a cpu model to use with the 'compound' host model");
    xbt_assert(not network_model_name.empty(), "Set a network model to use with the 'compound' host model");

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

  XBT_DEBUG("Call disk_model_init");
  int disk_id = find_model_description(surf_disk_model_description, disk_model_name);
  surf_disk_model_description[disk_id].model_init_preparse();

  XBT_DEBUG("Call storage_model_init");
  int storage_id = find_model_description(surf_storage_model_description, storage_model_name);
  surf_storage_model_description[storage_id].model_init_preparse();
}

/**
 * @brief Add a Zone to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host, router or sub-Zone) added after this call
 * and before the corresponding call to sg_platf_new_Zone_seal() will be added to this Zone.
 *
 * Once this function was called, the configuration concerning the used models cannot be changed anymore.
 *
 * @param zone the parameters defining the Zone to build.
 */
simgrid::kernel::routing::NetZoneImpl* sg_platf_new_Zone_begin(const simgrid::kernel::routing::ZoneCreationArgs* zone)
{
  if (not surf_parse_models_setup_already_called) {
    simgrid::s4u::Engine::on_platform_creation();

    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|zone|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer come down to zone creations, so putting this verification here is correct.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */

  /* search the routing model */
  simgrid::kernel::routing::NetZoneImpl* new_zone = nullptr;
  simgrid::kernel::resource::NetworkModel* netmodel =
      current_routing == nullptr ? surf_network_model : current_routing->network_model_;
  switch (zone->routing) {
    case A_surfxml_AS_routing_Cluster:
      new_zone = new simgrid::kernel::routing::ClusterZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_ClusterDragonfly:
      new_zone = new simgrid::kernel::routing::DragonflyZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_ClusterTorus:
      new_zone = new simgrid::kernel::routing::TorusZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_ClusterFatTree:
      new_zone = new simgrid::kernel::routing::FatTreeZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_Dijkstra:
      new_zone = new simgrid::kernel::routing::DijkstraZone(current_routing, zone->id, netmodel, false);
      break;
    case A_surfxml_AS_routing_DijkstraCache:
      new_zone = new simgrid::kernel::routing::DijkstraZone(current_routing, zone->id, netmodel, true);
      break;
    case A_surfxml_AS_routing_Floyd:
      new_zone = new simgrid::kernel::routing::FloydZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_Full:
      new_zone = new simgrid::kernel::routing::FullZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_None:
      new_zone = new simgrid::kernel::routing::EmptyZone(current_routing, zone->id, netmodel);
      break;
    case A_surfxml_AS_routing_Vivaldi:
      new_zone = new simgrid::kernel::routing::VivaldiZone(current_routing, zone->id, netmodel);
      break;
    default:
      xbt_die("Not a valid model!");
      break;
  }

  if (current_routing == nullptr) { /* it is the first one */
    simgrid::s4u::Engine::get_instance()->set_netzone_root(new_zone->get_iface());
  } else {
    /* set the father behavior */
    if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
      current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::recursive;
    /* add to the sons dictionary */
    current_routing->get_children()->push_back(new_zone);
  }

  /* set the new current component of the tree */
  current_routing = new_zone;
  simgrid::s4u::NetZone::on_creation(*new_zone->get_iface()); // notify the signal

  return new_zone;
}

void sg_platf_new_Zone_set_properties(const std::unordered_map<std::string, std::string>* props)
{
  xbt_assert(current_routing, "Cannot set properties of the current Zone: none under construction");

  if (props)
    current_routing->set_properties(*props);
}

/**
 * @brief Specify that the description of the current AS is finished
 *
 * Once you've declared all the content of your AS, you have to seal
 * it with this call. Your AS is not usable until you call this function.
 */
void sg_platf_new_Zone_seal()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->seal();
  simgrid::s4u::NetZone::on_seal(*current_routing->get_iface());
  current_routing = current_routing->get_father();
}

/** @brief Add a link connecting a host to the rest of its AS (which must be cluster or vivaldi) */
void sg_platf_new_hostlink(const simgrid::kernel::routing::HostLinkCreationArgs* hostlink)
{
  const simgrid::kernel::routing::NetPoint* netpoint = simgrid::s4u::Host::by_name(hostlink->id)->get_netpoint();
  xbt_assert(netpoint, "Host '%s' not found!", hostlink->id.c_str());
  xbt_assert(dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing),
             "Only hosts from Cluster and Vivaldi ASes can get a host_link.");

  const simgrid::s4u::Link* linkUp   = simgrid::s4u::Link::by_name_or_null(hostlink->link_up);
  const simgrid::s4u::Link* linkDown = simgrid::s4u::Link::by_name_or_null(hostlink->link_down);

  xbt_assert(linkUp, "Link '%s' not found!", hostlink->link_up.c_str());
  xbt_assert(linkDown, "Link '%s' not found!", hostlink->link_down.c_str());

  auto* as_cluster = static_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  if (as_cluster->private_links_.find(netpoint->id()) != as_cluster->private_links_.end())
    surf_parse_error(std::string("Host_link for '") + hostlink->id.c_str() + "' is already defined!");

  XBT_DEBUG("Push Host_link for host '%s' to position %u", netpoint->get_cname(), netpoint->id());
  as_cluster->private_links_.insert({netpoint->id(), {linkUp->get_impl(), linkDown->get_impl()}});
}

void sg_platf_new_trace(simgrid::kernel::routing::ProfileCreationArgs* profile)
{
  simgrid::kernel::profile::Profile* mgr_profile;
  if (not profile->file.empty()) {
    mgr_profile = simgrid::kernel::profile::Profile::from_file(profile->file);
  } else {
    xbt_assert(not profile->pc_data.empty(), "Trace '%s' must have either a content, or point to a file on disk.",
               profile->id.c_str());
    mgr_profile = simgrid::kernel::profile::Profile::from_string(profile->id, profile->pc_data, profile->periodicity);
  }
  traces_set_list.insert({profile->id, mgr_profile});
}
