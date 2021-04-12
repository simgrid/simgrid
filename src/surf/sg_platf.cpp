/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

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
#include "simgrid/kernel/routing/WifiZone.hpp"
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

namespace simgrid {
namespace kernel {
namespace routing {
xbt::signal<void(ClusterCreationArgs const&)> on_cluster_creation;
} // namespace routing
} // namespace kernel
} // namespace simgrid

/** The current NetZone in the parsing */
static simgrid::kernel::routing::NetZoneImpl* current_routing = nullptr;
static simgrid::kernel::routing::NetZoneImpl* routing_get_current()
{
  return current_routing;
}

/** Module management function: creates all internal data structures */
void sg_platf_init()
{
  // Do nothing: just for symmetry of user code
}

/** Module management function: frees all internal data structures */
void sg_platf_exit()
{
  simgrid::kernel::routing::on_cluster_creation.disconnect_slots();
  simgrid::s4u::Engine::on_platform_created.disconnect_slots();

  surf_parse_lex_destroy();
}

/** @brief Add a host to the current NetZone */
void sg_platf_new_host(const simgrid::kernel::routing::HostCreationArgs* args)
{
  simgrid::s4u::Host* host = routing_get_current()
                                 ->create_host(args->id, args->speed_per_pstate)
                                 ->set_coordinates(args->coord)
                                 ->set_properties(args->properties);

  host->get_impl()->set_disks(args->disks);

  host->pimpl_cpu->set_core_count(args->core_amount)
      ->set_state_profile(args->state_trace)
      ->set_speed_profile(args->speed_trace);

  host->seal();

  /* When energy plugin is activated, changing the pstate requires to already have the HostEnergy extension whose
   * allocation is triggered by the on_creation signal. Then set_pstate must be called after the signal emition */
  if (args->pstate != 0)
    host->set_pstate(args->pstate);
}

/** @brief Add a "router" to the network element list */
simgrid::kernel::routing::NetPoint* sg_platf_new_router(const std::string& name, const std::string& coords)
{
  xbt_assert(nullptr == simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(name),
             "Refusing to create a router named '%s': this name already describes a node.", name.c_str());

  auto* netpoint = new simgrid::kernel::routing::NetPoint(name, simgrid::kernel::routing::NetPoint::Type::Router);
  netpoint->set_englobing_zone(current_routing);
  XBT_DEBUG("Router '%s' has the id %u", netpoint->get_cname(), netpoint->id());

  if (not coords.empty())
    netpoint->set_coordinates(coords);

  return netpoint;
}

static void sg_platf_new_link(const simgrid::kernel::routing::LinkCreationArgs* args, const std::string& link_name)
{
  routing_get_current()
      ->create_link(link_name, args->bandwidths)
      ->set_properties(args->properties)
      ->get_impl() // this call to get_impl saves some simcalls but can be removed
      ->set_sharing_policy(args->policy)
      ->set_state_profile(args->state_trace)
      ->set_latency_profile(args->latency_trace)
      ->set_bandwidth_profile(args->bandwidth_trace)
      ->set_latency(args->latency)
      ->seal();
}

void sg_platf_new_link(const simgrid::kernel::routing::LinkCreationArgs* link)
{
  if (link->policy == simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX) {
    sg_platf_new_link(link, link->id + "_UP");
    sg_platf_new_link(link, link->id + "_DOWN");
  } else {
    sg_platf_new_link(link, link->id);
  }
}

void sg_platf_new_cluster(simgrid::kernel::routing::ClusterCreationArgs* cluster)
{
  using simgrid::kernel::routing::ClusterZone;
  using simgrid::kernel::routing::DragonflyZone;
  using simgrid::kernel::routing::FatTreeZone;
  using simgrid::kernel::routing::TorusZone;

  int rankId = 0;

  // What an inventive way of initializing the NetZone that I have as ancestor :-(
  simgrid::kernel::routing::ZoneCreationArgs zone;
  zone.id = cluster->id;
  switch (cluster->topology) {
    case simgrid::kernel::routing::ClusterTopology::TORUS:
      zone.routing = "ClusterTorus";
      break;
    case simgrid::kernel::routing::ClusterTopology::DRAGONFLY:
      zone.routing = "ClusterDragonfly";
      break;
    case simgrid::kernel::routing::ClusterTopology::FAT_TREE:
      zone.routing = "ClusterFatTree";
      break;
    default:
      zone.routing = "Cluster";
      break;
  }
  sg_platf_new_Zone_begin(&zone);
  auto* current_zone = static_cast<ClusterZone*>(routing_get_current());
  current_zone->parse_specific_arguments(cluster);
  for (auto const& elm : cluster->properties)
    current_zone->get_iface()->set_property(elm.first, elm.second);

  if (cluster->loopback_bw > 0 || cluster->loopback_lat > 0) {
    current_zone->set_loopback();
  }

  if (cluster->limiter_link > 0) {
    current_zone->set_limiter();
  }

  for (int const& i : cluster->radicals) {
    std::string host_id = std::string(cluster->prefix) + std::to_string(i) + cluster->suffix;

    XBT_DEBUG("<host\tid=\"%s\"\tspeed=\"%f\">", host_id.c_str(), cluster->speeds.front());
    current_zone->create_host(host_id, cluster->speeds)
        ->set_core_count(cluster->core_amount)
        ->set_properties(cluster->properties)
        ->seal();

    XBT_DEBUG("</host>");

    std::string link_id = std::string(cluster->id) + "_link_" + std::to_string(i);
    XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id.c_str(), cluster->bw, cluster->lat);

    // All links are saved in a matrix;
    // every row describes a single node; every node may have multiple links.
    // the first column may store a link from x to x if p_has_loopback is set
    // the second column may store a limiter link if p_has_limiter is set
    // other columns are to store one or more link for the node

    // add a loopback link
    if (cluster->loopback_bw > 0 || cluster->loopback_lat > 0) {
      std::string loopback_name = link_id + "_loopback";
      XBT_DEBUG("<loopback\tid=\"%s\"\tbw=\"%f\"/>", loopback_name.c_str(), cluster->loopback_bw);

      auto* loopback = current_zone->create_link(loopback_name, std::vector<double>{cluster->loopback_bw})
                           ->set_sharing_policy(simgrid::s4u::Link::SharingPolicy::FATPIPE)
                           ->set_latency(cluster->loopback_lat)
                           ->seal()
                           ->get_impl();

      current_zone->add_private_link_at(current_zone->node_pos(rankId), {loopback, loopback});
    }

    // add a limiter link (shared link to account for maximal bandwidth of the node)
    if (cluster->limiter_link > 0) {
      std::string limiter_name = std::string(link_id) + "_limiter";
      XBT_DEBUG("<limiter\tid=\"%s\"\tbw=\"%f\"/>", limiter_name.c_str(), cluster->limiter_link);

      auto* limiter =
          current_zone->create_link(limiter_name, std::vector<double>{cluster->limiter_link})->seal()->get_impl();

      current_zone->add_private_link_at(current_zone->node_pos_with_loopback(rankId), {limiter, limiter});
    }

    // call the cluster function that adds the others links
    if (cluster->topology == simgrid::kernel::routing::ClusterTopology::FAT_TREE) {
      static_cast<FatTreeZone*>(current_zone)->add_processing_node(i);
    } else {
      current_zone->create_links_for_node(cluster, i, rankId, current_zone->node_pos_with_loopback_limiter(rankId));
    }
    rankId++;
  }

  // Add a router.
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id.c_str());
  if (cluster->router_id.empty())
    cluster->router_id = std::string(cluster->prefix) + cluster->id + "_router" + cluster->suffix;
  current_zone->set_router(sg_platf_new_router(cluster->router_id, ""));

  // Make the backbone
  if ((cluster->bb_bw > 0) || (cluster->bb_lat > 0)) {
    std::string bb_name = std::string(cluster->id) + "_backbone";
    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/> <!--backbone -->", bb_name.c_str(), cluster->bb_bw,
              cluster->bb_lat);

    auto* backbone = current_zone->create_link(bb_name, std::vector<double>{cluster->bb_bw})
                         ->set_sharing_policy(cluster->bb_sharing_policy)
                         ->set_latency(cluster->bb_lat)
                         ->seal();
    current_zone->set_backbone(backbone->get_impl());
  }

  XBT_DEBUG("</zone>");
  sg_platf_new_Zone_seal();

  simgrid::kernel::routing::on_cluster_creation(*cluster);
}

void routing_cluster_add_backbone(simgrid::kernel::resource::LinkImpl* bb)
{
  auto* cluster = dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(not cluster->has_backbone(), "Cluster %s already has a backbone link!", cluster->get_cname());

  cluster->set_backbone(bb);
  XBT_DEBUG("Add a backbone to zone '%s'", current_routing->get_cname());
}

void sg_platf_new_cabinet(const simgrid::kernel::routing::CabinetCreationArgs* cabinet)
{
  auto* zone = static_cast<simgrid::kernel::routing::ClusterZone*>(routing_get_current());
  for (int const& radical : cabinet->radicals) {
    std::string id           = cabinet->prefix + std::to_string(radical) + cabinet->suffix;
    simgrid::s4u::Host* host = zone->create_host(id, std::vector<double>{cabinet->speed})->seal();

    simgrid::s4u::Link* link_up =
        zone->create_link("link_" + id + "_UP", std::vector<double>{cabinet->bw})->set_latency(cabinet->lat)->seal();
    simgrid::s4u::Link* link_down =
        zone->create_link("link_" + id + "_DOWN", std::vector<double>{cabinet->bw})->set_latency(cabinet->lat)->seal();

    zone->add_private_link_at(host->get_netpoint()->id(), {link_up->get_impl(), link_down->get_impl()});
  }
}

simgrid::kernel::resource::DiskImpl* sg_platf_new_disk(const simgrid::kernel::routing::DiskCreationArgs* disk)
{
  simgrid::kernel::resource::DiskImpl* pimpl = routing_get_current()
                                                   ->create_disk(disk->id, disk->read_bw, disk->write_bw)
                                                   ->set_properties(disk->properties)
                                                   ->get_impl();

  pimpl->seal();
  simgrid::s4u::Disk::on_creation(*pimpl->get_iface());
  return pimpl;
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

  std::string actor_name                 = actor->args[0];
  simgrid::kernel::actor::ActorCode code = factory(std::move(actor->args));

  auto* arg = new simgrid::kernel::actor::ProcessArg(actor_name, code, nullptr, host, kill_time, actor->properties,
                                                     auto_restart);

  host->get_impl()->add_actor_at_boot(arg);

  if (start_time > SIMIX_get_clock()) {
    arg = new simgrid::kernel::actor::ProcessArg(actor_name, code, nullptr, host, kill_time, actor->properties,
                                                 auto_restart);

    XBT_DEBUG("Process %s@%s will be started at time %f", arg->name.c_str(), arg->host->get_cname(), start_time);
    simgrid::simix::Timer::set(start_time, [arg, auto_restart]() {
      simgrid::kernel::actor::ActorImplPtr new_actor = simgrid::kernel::actor::ActorImpl::create(
          arg->name.c_str(), arg->code, arg->data, arg->host, arg->properties, nullptr);
      if (arg->kill_time >= 0)
        new_actor->set_kill_time(arg->kill_time);
      if (auto_restart)
        new_actor->set_auto_restart(auto_restart);
      delete arg;
    });
  } else { // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", arg->name.c_str(), host->get_cname());

    try {
      simgrid::kernel::actor::ActorImplPtr new_actor = nullptr;
      new_actor =
          simgrid::kernel::actor::ActorImpl::create(arg->name.c_str(), code, nullptr, host, arg->properties, nullptr);
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
  auto* zone = dynamic_cast<simgrid::kernel::routing::VivaldiZone*>(current_routing);
  xbt_assert(zone, "<peer> tag can only be used in Vivaldi netzones.");

  simgrid::s4u::Host* host = zone->create_host(peer->id, std::vector<double>{peer->speed})
                                 ->set_state_profile(peer->state_trace)
                                 ->set_speed_profile(peer->speed_trace)
                                 ->set_coordinates(peer->coord)
                                 ->seal();

  zone->set_peer_link(host->get_netpoint(), peer->bw_in, peer->bw_out);
}
/**
 * @brief Auxiliary function to build the object NetZoneImpl
 *
 * Builds the objects, setting its father properties and root netzone if needed
 * @param zone the parameters defining the Zone to build.
 * @return Pointer to recently created netzone
 */
static simgrid::kernel::routing::NetZoneImpl*
sg_platf_create_zone(const simgrid::kernel::routing::ZoneCreationArgs* zone)
{
  /* search the routing model */
  simgrid::kernel::routing::NetZoneImpl* new_zone = nullptr;

  if (strcasecmp(zone->routing.c_str(), "Cluster") == 0) {
    new_zone = new simgrid::kernel::routing::ClusterZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "ClusterDragonfly") == 0) {
    new_zone = new simgrid::kernel::routing::DragonflyZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "ClusterTorus") == 0) {
    new_zone = new simgrid::kernel::routing::TorusZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "ClusterFatTree") == 0) {
    new_zone = new simgrid::kernel::routing::FatTreeZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "Dijkstra") == 0) {
    new_zone = new simgrid::kernel::routing::DijkstraZone(zone->id, false);
  } else if (strcasecmp(zone->routing.c_str(), "DijkstraCache") == 0) {
    new_zone = new simgrid::kernel::routing::DijkstraZone(zone->id, true);
  } else if (strcasecmp(zone->routing.c_str(), "Floyd") == 0) {
    new_zone = new simgrid::kernel::routing::FloydZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "Full") == 0) {
    new_zone = new simgrid::kernel::routing::FullZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "None") == 0) {
    new_zone = new simgrid::kernel::routing::EmptyZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "Vivaldi") == 0) {
    new_zone = new simgrid::kernel::routing::VivaldiZone(zone->id);
  } else if (strcasecmp(zone->routing.c_str(), "Wifi") == 0) {
    new_zone = new simgrid::kernel::routing::WifiZone(zone->id);
  } else {
    xbt_die("Not a valid model!");
  }
  new_zone->set_parent(current_routing);

  return new_zone;
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
  /* First create the zone.
   * This order is important to assure that root netzone is set when models are setting
   * the default mode for each resource (CPU, network, etc)
   */
  auto* new_zone = sg_platf_create_zone(zone);

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */

  /* set the new current component of the tree */
  current_routing = new_zone;
  simgrid::s4u::NetZone::on_creation(*new_zone->get_iface()); // notify the signal

  return new_zone;
}

void sg_platf_new_Zone_set_properties(const std::unordered_map<std::string, std::string>& props)
{
  xbt_assert(current_routing, "Cannot set properties of the current Zone: none under construction");

  current_routing->set_properties(props);
}

/**
 * @brief Specify that the description of the current Zone is finished
 *
 * Once you've declared all the content of your Zone, you have to seal
 * it with this call. Your Zone is not usable until you call this function.
 */
void sg_platf_new_Zone_seal()
{
  xbt_assert(current_routing, "Cannot seal the current Zone: zone under construction");
  current_routing->seal();
  simgrid::s4u::NetZone::on_seal(*current_routing->get_iface());
  current_routing = current_routing->get_parent();
}

/** @brief Add a link connecting a host to the rest of its Zone (which must be cluster or vivaldi) */
void sg_platf_new_hostlink(const simgrid::kernel::routing::HostLinkCreationArgs* hostlink)
{
  const simgrid::kernel::routing::NetPoint* netpoint = simgrid::s4u::Host::by_name(hostlink->id)->get_netpoint();
  xbt_assert(netpoint, "Host '%s' not found!", hostlink->id.c_str());
  xbt_assert(dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing),
             "Only hosts from Cluster and Vivaldi Zones can get a host_link.");

  const simgrid::s4u::Link* linkUp   = simgrid::s4u::Link::by_name_or_null(hostlink->link_up);
  const simgrid::s4u::Link* linkDown = simgrid::s4u::Link::by_name_or_null(hostlink->link_down);

  xbt_assert(linkUp, "Link '%s' not found!", hostlink->link_up.c_str());
  xbt_assert(linkDown, "Link '%s' not found!", hostlink->link_down.c_str());

  auto* cluster_zone = static_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  if (cluster_zone->private_link_exists_at(netpoint->id()))
    surf_parse_error(std::string("Host_link for '") + hostlink->id.c_str() + "' is already defined!");

  XBT_DEBUG("Push Host_link for host '%s' to position %u", netpoint->get_cname(), netpoint->id());
  cluster_zone->add_private_link_at(netpoint->id(), {linkUp->get_impl(), linkDown->get_impl()});
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
