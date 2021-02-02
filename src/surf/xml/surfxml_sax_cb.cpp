/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/file.hpp"
#include "xbt/parse_units.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "simgrid_dtd.c"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf, "Logging specific to the SURF parsing module");

std::string surf_parsed_filename; // Currently parsed file (for the error messages)
std::vector<simgrid::kernel::resource::LinkImpl*>
    parsed_link_list; /* temporary store of current link list of a route */
std::vector<simgrid::kernel::resource::DiskImpl*> parsed_disk_list; /* temporary store of current disk list of a host */
/*
 * Helping functions
 */
void surf_parse_assert(bool cond, const std::string& msg)
{
  if (not cond)
    surf_parse_error(msg);
}

void surf_parse_error(const std::string& msg)
{
  throw simgrid::ParseError(surf_parsed_filename, surf_parse_lineno, msg);
}

void surf_parse_assert_netpoint(const std::string& hostname, const std::string& pre, const std::string& post)
{
  if (simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(hostname) != nullptr) // found
    return;

  std::string msg = pre + hostname + post + " Existing netpoints: \n";

  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      simgrid::s4u::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });
  bool first = true;
  for (auto const& np : netpoints) {
    if (np->is_netzone())
      continue;

    if (not first)
      msg += ",";
    first = false;
    msg += "'" + np->get_name() + "'";
    if (msg.length() > 4096) {
      msg.pop_back(); // remove trailing quote
      msg += "...(list truncated)......";
      break;
    }
  }
  surf_parse_error(msg);
}

double surf_parse_get_double(const std::string& s)
{
  try {
    return std::stod(s);
  } catch (const std::invalid_argument&) {
    surf_parse_error(s + " is not a double");
  }
}

int surf_parse_get_int(const std::string& s)
{
  try {
    return std::stoi(s);
  } catch (const std::invalid_argument&) {
    surf_parse_error(s + " is not a double");
  }
}

/* Turn something like "1-4,6,9-11" into the vector {1,2,3,4,6,9,10,11} */
static std::vector<int>* explodesRadical(const std::string& radicals)
{
  auto* exploded = new std::vector<int>();

  // Make all hosts
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, radicals, boost::is_any_of(","));
  for (auto const& group : radical_elements) {
    std::vector<std::string> radical_ends;
    boost::split(radical_ends, group, boost::is_any_of("-"));
    int start = surf_parse_get_int(radical_ends.front());
    int end   = 0;

    switch (radical_ends.size()) {
      case 1:
        end = start;
        break;
      case 2:
        end = surf_parse_get_int(radical_ends.back());
        break;
      default:
        surf_parse_error(std::string("Malformed radical: ") + group);
    }
    for (int i = start; i <= end; i++)
      exploded->push_back(i);
  }

  return exploded;
}


/*
 * All the callback lists that can be overridden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */

std::vector<std::unordered_map<std::string, std::string>*> property_sets;

/* The default current property receiver. Setup in the corresponding opening callbacks. */
std::unordered_map<std::string, std::string>* current_model_property_set = nullptr;

FILE *surf_file_to_parse = nullptr;

/* Stuff relative to storage */
void STag_surfxml_storage()
{
  xbt_die("<storage> tag was removed in SimGrid v3.27. Please stop using it now.");
}

void ETag_surfxml_storage()
{
  /* Won't happen since <storage> is now removed since v3.27. */
}
void STag_surfxml_storage___type()
{
  xbt_die("<storage_type> tag was removed in SimGrid v3.27. Please stop using it now.");
}
void ETag_surfxml_storage___type()
{
  /* Won't happen since <storage_type> is now removed since v3.27. */
}

void STag_surfxml_mount()
{
  xbt_die("<mount> tag was removed in SimGrid v3.27. Please stop using it now.");
}

void ETag_surfxml_mount()
{
  /* Won't happen since <mount> is now removed since v3.27. */
}

void STag_surfxml_include()
{
  xbt_die("<include> tag was removed in SimGrid v3.18. Please stop using it now.");
}

void ETag_surfxml_include()
{
  /* Won't happen since <include> is now removed since v3.18. */
}

/* Stag and Etag parse functions */
void STag_surfxml_platform() {
  double version = surf_parse_get_double(A_surfxml_platform_version);

  surf_parse_assert((version >= 1.0), "******* BIG FAT WARNING *********\n "
      "You're using an ancient XML file.\n"
      "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
      "instead of MBytes, MFlops and seconds.\n"

      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.\n"

      "Please check also out the SURF section of the ChangeLog for "
      "the 3.1 version for more information. \n"

      "Last, do not forget to also update your values for "
      "the calls to MSG_task_create (if any).");
  surf_parse_assert((version >= 3.0), "******* BIG FAT WARNING *********\n "
      "You're using an old XML file.\n"
      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.");
  surf_parse_assert((version >= 4.0),
             std::string("******* THIS FILE IS TOO OLD (v:")+std::to_string(version)+") *********\n "
             "Changes introduced in SimGrid 3.13:\n"
             "  - 'power' attribute of hosts (and others) got renamed to 'speed'.\n"
             "  - In <trace_connect>, attribute kind=\"POWER\" is now kind=\"SPEED\".\n"
             "  - DOCTYPE now point to the rignt URL.\n"
             "  - speed, bandwidth and latency attributes now MUST have an explicit unit (f, Bps, s by default)"
             "\n\n"
             "Use simgrid_update_xml to update your file automatically. "
             "This program is installed automatically with SimGrid, or "
             "available in the tools/ directory of the source archive.");
  if (version < 4.1) {
    XBT_INFO("You're using a v%.1f XML file (%s) while the current standard is v4.1 "
             "That's fine, the new version is backward compatible. \n\n"
             "Use simgrid_update_xml to update your file automatically to get rid of this warning. "
             "This program is installed automatically with SimGrid, or "
             "available in the tools/ directory of the source archive.",
             version, surf_parsed_filename.c_str());
  }
  surf_parse_assert(version <= 4.1,
             std::string("******* THIS FILE COMES FROM THE FUTURE (v:")+std::to_string(version)+") *********\n "
             "The most recent formalism that this version of SimGrid understands is v4.1.\n"
             "Please update your code, or use another, more adapted, file.");
}
void ETag_surfxml_platform(){
  simgrid::s4u::Engine::on_platform_created();
}

void STag_surfxml_host(){
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
}

void STag_surfxml_prop()
{
  property_sets.back()->insert({A_surfxml_prop_id, A_surfxml_prop_value});
  XBT_DEBUG("add prop %s=%s into current property set %p", A_surfxml_prop_id, A_surfxml_prop_value,
            property_sets.back());
}

void ETag_surfxml_host()    {
  simgrid::kernel::routing::HostCreationArgs host;

  host.properties = property_sets.back();
  property_sets.pop_back();

  host.id = A_surfxml_host_id;

  host.speed_per_pstate =
      xbt_parse_get_all_speeds(surf_parsed_filename, surf_parse_lineno, A_surfxml_host_speed, "speed of host", host.id);

  XBT_DEBUG("pstate: %s", A_surfxml_host_pstate);
  host.core_amount = surf_parse_get_int(A_surfxml_host_core);

  if (A_surfxml_host_availability___file[0] != '\0') {
    XBT_WARN("The availability_file attribute in <host> is now deprecated. Please, use 'speed_file' instead.");
    host.speed_trace = simgrid::kernel::profile::Profile::from_file(A_surfxml_host_availability___file);
  }
  if (A_surfxml_host_speed___file[0] != '\0')
    host.speed_trace = simgrid::kernel::profile::Profile::from_file(A_surfxml_host_speed___file);
  host.state_trace = A_surfxml_host_state___file[0]
                         ? simgrid::kernel::profile::Profile::from_file(A_surfxml_host_state___file)
                         : nullptr;
  host.pstate      = surf_parse_get_int(A_surfxml_host_pstate);
  host.coord       = A_surfxml_host_coordinates;
  host.disks.swap(parsed_disk_list);

  sg_platf_new_host(&host);
}

void STag_surfxml_disk() {
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
}

void ETag_surfxml_disk() {
  simgrid::kernel::routing::DiskCreationArgs disk;
  disk.properties = property_sets.back();
  property_sets.pop_back();

  disk.id       = A_surfxml_disk_id;
  disk.read_bw  = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_disk_read___bw,
                                         "read_bw of disk ", disk.id);
  disk.write_bw = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_disk_write___bw,
                                          "write_bw of disk ", disk.id);

  parsed_disk_list.push_back(sg_platf_new_disk(&disk));
}

void STag_surfxml_host___link(){
  XBT_DEBUG("Create a Host_link for %s",A_surfxml_host___link_id);
  simgrid::kernel::routing::HostLinkCreationArgs host_link;

  host_link.id        = A_surfxml_host___link_id;
  host_link.link_up   = A_surfxml_host___link_up;
  host_link.link_down = A_surfxml_host___link_down;
  sg_platf_new_hostlink(&host_link);
}

void STag_surfxml_router(){
  sg_platf_new_router(A_surfxml_router_id, A_surfxml_router_coordinates);
}

void ETag_surfxml_cluster(){
  simgrid::kernel::routing::ClusterCreationArgs cluster;
  cluster.properties = property_sets.back();
  property_sets.pop_back();

  cluster.id          = A_surfxml_cluster_id;
  cluster.prefix      = A_surfxml_cluster_prefix;
  cluster.suffix      = A_surfxml_cluster_suffix;
  cluster.radicals    = explodesRadical(A_surfxml_cluster_radical);
  cluster.speeds      = xbt_parse_get_all_speeds(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_speed,
                                            "speed of cluster", cluster.id);
  cluster.core_amount = surf_parse_get_int(A_surfxml_cluster_core);
  cluster.bw = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_bw, "bw of cluster",
                                       cluster.id);
  cluster.lat =
      xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_lat, "lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___bw,""))
    cluster.bb_bw = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_bb___bw,
                                            "bb_bw of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___lat,""))
    cluster.bb_lat = xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_bb___lat,
                                        "bb_lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_limiter___link,""))
    cluster.limiter_link =
        xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_limiter___link,
                                "limiter_link of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_loopback___bw,""))
    cluster.loopback_bw = xbt_parse_get_bandwidth(
        surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_loopback___bw, "loopback_bw of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_loopback___lat,""))
    cluster.loopback_lat = xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_cluster_loopback___lat,
                                              "loopback_lat of cluster", cluster.id);

  switch(AX_surfxml_cluster_topology){
  case A_surfxml_cluster_topology_FLAT:
    cluster.topology = simgrid::kernel::routing::ClusterTopology::FLAT;
    break;
  case A_surfxml_cluster_topology_TORUS:
    cluster.topology = simgrid::kernel::routing::ClusterTopology::TORUS;
    break;
  case A_surfxml_cluster_topology_FAT___TREE:
    cluster.topology = simgrid::kernel::routing::ClusterTopology::FAT_TREE;
    break;
  case A_surfxml_cluster_topology_DRAGONFLY:
    cluster.topology = simgrid::kernel::routing::ClusterTopology::DRAGONFLY;
    break;
  default:
    surf_parse_error(std::string("Invalid cluster topology for cluster ") + cluster.id);
  }
  cluster.topo_parameters = A_surfxml_cluster_topo___parameters;
  cluster.router_id = A_surfxml_cluster_router___id;

  switch (AX_surfxml_cluster_sharing___policy) {
  case A_surfxml_cluster_sharing___policy_SHARED:
    cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SHARED;
    break;
  case A_surfxml_cluster_sharing___policy_FULLDUPLEX:
    XBT_WARN("FULLDUPLEX is now deprecated. Please update your platform file to use SPLITDUPLEX instead.");
    cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
    break;
  case A_surfxml_cluster_sharing___policy_SPLITDUPLEX:
    cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
    break;
  case A_surfxml_cluster_sharing___policy_FATPIPE:
    cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
    break;
  default:
    surf_parse_error(std::string("Invalid cluster sharing policy for cluster ") + cluster.id);
  }
  switch (AX_surfxml_cluster_bb___sharing___policy) {
  case A_surfxml_cluster_bb___sharing___policy_FATPIPE:
    cluster.bb_sharing_policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
    break;
  case A_surfxml_cluster_bb___sharing___policy_SHARED:
    cluster.bb_sharing_policy = simgrid::s4u::Link::SharingPolicy::SHARED;
    break;
  default:
    surf_parse_error(std::string("Invalid bb sharing policy in cluster ") + cluster.id);
  }

  sg_platf_new_cluster(&cluster);
}

void STag_surfxml_cluster(){
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
}

void STag_surfxml_cabinet(){
  simgrid::kernel::routing::CabinetCreationArgs cabinet;
  cabinet.id      = A_surfxml_cabinet_id;
  cabinet.prefix  = A_surfxml_cabinet_prefix;
  cabinet.suffix  = A_surfxml_cabinet_suffix;
  cabinet.speed   = xbt_parse_get_speed(surf_parsed_filename, surf_parse_lineno, A_surfxml_cabinet_speed,
                                      "speed of cabinet", cabinet.id.c_str());
  cabinet.bw  = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_cabinet_bw, "bw of cabinet",
                                       cabinet.id.c_str());
  cabinet.lat = xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_cabinet_lat, "lat of cabinet",
                                   cabinet.id.c_str());
  cabinet.radicals = explodesRadical(A_surfxml_cabinet_radical);

  sg_platf_new_cabinet(&cabinet);
}

void STag_surfxml_peer(){
  simgrid::kernel::routing::PeerCreationArgs peer;

  peer.id          = std::string(A_surfxml_peer_id);
  peer.speed       = xbt_parse_get_speed(surf_parsed_filename, surf_parse_lineno, A_surfxml_peer_speed, "speed of peer",
                                   peer.id.c_str());
  peer.bw_in = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_peer_bw___in, "bw_in of peer",
                                       peer.id.c_str());
  peer.bw_out      = xbt_parse_get_bandwidth(surf_parsed_filename, surf_parse_lineno, A_surfxml_peer_bw___out,
                                        "bw_out of peer", peer.id.c_str());
  peer.coord       = A_surfxml_peer_coordinates;
  peer.speed_trace = nullptr;
  if (A_surfxml_peer_availability___file[0] != '\0') {
    XBT_WARN("The availability_file attribute in <peer> is now deprecated. Please, use 'speed_file' instead.");
    peer.speed_trace = simgrid::kernel::profile::Profile::from_file(A_surfxml_peer_availability___file);
  }
  if (A_surfxml_peer_speed___file[0] != '\0')
    peer.speed_trace = simgrid::kernel::profile::Profile::from_file(A_surfxml_peer_speed___file);
  peer.state_trace = A_surfxml_peer_state___file[0]
                         ? simgrid::kernel::profile::Profile::from_file(A_surfxml_peer_state___file)
                         : nullptr;

  if (A_surfxml_peer_lat[0] != '\0')
    XBT_WARN("The latency attribute in <peer> is now deprecated. Use the z coordinate instead of '%s'.",
             A_surfxml_peer_lat);

  sg_platf_new_peer(&peer);
}

void STag_surfxml_link(){
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
}

void ETag_surfxml_link(){
  simgrid::kernel::routing::LinkCreationArgs link;

  link.properties = property_sets.back();
  property_sets.pop_back();

  link.id                  = std::string(A_surfxml_link_id);
  link.bandwidths          = xbt_parse_get_bandwidths(surf_parsed_filename, surf_parse_lineno, A_surfxml_link_bandwidth,
                                             "bandwidth of link", link.id.c_str());
  link.bandwidth_trace     = A_surfxml_link_bandwidth___file[0]
                             ? simgrid::kernel::profile::Profile::from_file(A_surfxml_link_bandwidth___file)
                             : nullptr;
  link.latency = xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_link_latency, "latency of link",
                                    link.id.c_str());
  link.latency_trace       = A_surfxml_link_latency___file[0]
                           ? simgrid::kernel::profile::Profile::from_file(A_surfxml_link_latency___file)
                           : nullptr;
  link.state_trace = A_surfxml_link_state___file[0]
                         ? simgrid::kernel::profile::Profile::from_file(A_surfxml_link_state___file)
                         : nullptr;

  switch (A_surfxml_link_sharing___policy) {
  case A_surfxml_link_sharing___policy_SHARED:
    link.policy = simgrid::s4u::Link::SharingPolicy::SHARED;
    break;
  case A_surfxml_link_sharing___policy_FATPIPE:
    link.policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
    break;
  case A_surfxml_link_sharing___policy_FULLDUPLEX:
    XBT_WARN("FULLDUPLEX is now deprecated. Please update your platform file to use SPLITDUPLEX instead.");
    link.policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
    break;
  case A_surfxml_link_sharing___policy_SPLITDUPLEX:
    link.policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
    break;
  case A_surfxml_link_sharing___policy_WIFI:
    link.policy = simgrid::s4u::Link::SharingPolicy::WIFI;
    break;
  default:
    surf_parse_error(std::string("Invalid sharing policy in link ") + link.id);
  }

  sg_platf_new_link(&link);
}

void STag_surfxml_link___ctn()
{
  simgrid::kernel::resource::LinkImpl* link = nullptr;
  switch (A_surfxml_link___ctn_direction) {
  case AU_surfxml_link___ctn_direction:
  case A_surfxml_link___ctn_direction_NONE:
    link = simgrid::s4u::Link::by_name(std::string(A_surfxml_link___ctn_id))->get_impl();
    break;
  case A_surfxml_link___ctn_direction_UP:
    link = simgrid::s4u::Link::by_name(std::string(A_surfxml_link___ctn_id) + "_UP")->get_impl();
    break;
  case A_surfxml_link___ctn_direction_DOWN:
    link = simgrid::s4u::Link::by_name(std::string(A_surfxml_link___ctn_id) + "_DOWN")->get_impl();
    break;
  default:
    surf_parse_error(std::string("Invalid direction for link ") + A_surfxml_link___ctn_id);
  }

  const char* dirname;
  switch (A_surfxml_link___ctn_direction) {
    case A_surfxml_link___ctn_direction_UP:
      dirname = " (upward)";
      break;
    case A_surfxml_link___ctn_direction_DOWN:
      dirname = " (downward)";
      break;
    default:
      dirname = "";
  }
  surf_parse_assert(link != nullptr, std::string("No such link: '") + A_surfxml_link___ctn_id + "'" + dirname);
  parsed_link_list.push_back(link);
}

void ETag_surfxml_backbone(){
  simgrid::kernel::routing::LinkCreationArgs link;

  link.id = std::string(A_surfxml_backbone_id);
  link.bandwidths.push_back(xbt_parse_get_bandwidth(
      surf_parsed_filename, surf_parse_lineno, A_surfxml_backbone_bandwidth, "bandwidth of backbone", link.id.c_str()));
  link.latency    = xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, A_surfxml_backbone_latency,
                                    "latency of backbone", link.id.c_str());
  link.policy     = simgrid::s4u::Link::SharingPolicy::SHARED;

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(simgrid::s4u::Link::by_name(std::string(A_surfxml_backbone_id))->get_impl());
}

void STag_surfxml_route(){
  surf_parse_assert_netpoint(A_surfxml_route_src, "Route src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_route_dst, "Route dst='", "' does name a node.");
}

void STag_surfxml_ASroute(){
  surf_parse_assert_netpoint(A_surfxml_ASroute_src, "ASroute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_ASroute_dst, "ASroute dst='", "' does name a node.");

  surf_parse_assert_netpoint(A_surfxml_ASroute_gw___src, "ASroute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_ASroute_gw___dst, "ASroute gw_dst='", "' does name a node.");
}
void STag_surfxml_zoneRoute(){
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_src, "zoneRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_dst, "zoneRoute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_gw___src, "zoneRoute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_gw___dst, "zoneRoute gw_dst='", "' does name a node.");
}

void STag_surfxml_bypassRoute(){
  surf_parse_assert_netpoint(A_surfxml_bypassRoute_src, "bypassRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassRoute_dst, "bypassRoute dst='", "' does name a node.");
}

void STag_surfxml_bypassASroute(){
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_src, "bypassASroute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_dst, "bypassASroute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_gw___src, "bypassASroute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_gw___dst, "bypassASroute gw_dst='", "' does name a node.");
}
void STag_surfxml_bypassZoneRoute(){
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_src, "bypassZoneRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_dst, "bypassZoneRoute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_gw___src, "bypassZoneRoute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_gw___dst, "bypassZoneRoute gw_dst='", "' does name a node.");
}

void ETag_surfxml_route(){
  simgrid::kernel::routing::RouteCreationArgs route;

  route.src         = sg_netpoint_by_name_or_null(A_surfxml_route_src); // tested to not be nullptr in start tag
  route.dst         = sg_netpoint_by_name_or_null(A_surfxml_route_dst); // tested to not be nullptr in start tag
  route.symmetrical = (A_surfxml_route_symmetrical == AU_surfxml_route_symmetrical ||
                       A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES ||
                       A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_yes);

  route.link_list.swap(parsed_link_list);

  sg_platf_new_route(&route);
}

void ETag_surfxml_ASroute()
{
  AX_surfxml_zoneRoute_src = AX_surfxml_ASroute_src;
  AX_surfxml_zoneRoute_dst = AX_surfxml_ASroute_dst;
  AX_surfxml_zoneRoute_gw___src = AX_surfxml_ASroute_gw___src;
  AX_surfxml_zoneRoute_gw___dst = AX_surfxml_ASroute_gw___dst;
  AX_surfxml_zoneRoute_symmetrical = (AT_surfxml_zoneRoute_symmetrical)AX_surfxml_ASroute_symmetrical;
  ETag_surfxml_zoneRoute();
}
void ETag_surfxml_zoneRoute()
{
  simgrid::kernel::routing::RouteCreationArgs ASroute;

  ASroute.src = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_src); // tested to not be nullptr in start tag
  ASroute.dst = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_dst); // tested to not be nullptr in start tag

  ASroute.gw_src = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_gw___src); // tested to not be nullptr in start tag
  ASroute.gw_dst = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_gw___dst); // tested to not be nullptr in start tag

  ASroute.link_list.swap(parsed_link_list);

  ASroute.symmetrical = (A_surfxml_zoneRoute_symmetrical == AU_surfxml_zoneRoute_symmetrical ||
                         A_surfxml_zoneRoute_symmetrical == A_surfxml_zoneRoute_symmetrical_YES ||
                         A_surfxml_zoneRoute_symmetrical == A_surfxml_zoneRoute_symmetrical_yes);

  sg_platf_new_route(&ASroute);
}

void ETag_surfxml_bypassRoute(){
  simgrid::kernel::routing::RouteCreationArgs route;

  route.src         = sg_netpoint_by_name_or_null(A_surfxml_bypassRoute_src); // tested to not be nullptr in start tag
  route.dst         = sg_netpoint_by_name_or_null(A_surfxml_bypassRoute_dst); // tested to not be nullptr in start tag
  route.symmetrical = false;

  route.link_list.swap(parsed_link_list);

  sg_platf_new_bypassRoute(&route);
}

void ETag_surfxml_bypassASroute()
{
  AX_surfxml_bypassZoneRoute_src = AX_surfxml_bypassASroute_src;
  AX_surfxml_bypassZoneRoute_dst = AX_surfxml_bypassASroute_dst;
  AX_surfxml_bypassZoneRoute_gw___src = AX_surfxml_bypassASroute_gw___src;
  AX_surfxml_bypassZoneRoute_gw___dst = AX_surfxml_bypassASroute_gw___dst;
  ETag_surfxml_bypassZoneRoute();
}
void ETag_surfxml_bypassZoneRoute()
{
  simgrid::kernel::routing::RouteCreationArgs ASroute;

  ASroute.src         = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_src);
  ASroute.dst         = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_dst);
  ASroute.link_list.swap(parsed_link_list);

  ASroute.symmetrical = false;

  ASroute.gw_src = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_gw___src);
  ASroute.gw_dst = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_gw___dst);

  sg_platf_new_bypassRoute(&ASroute);
}

void ETag_surfxml_trace(){
  simgrid::kernel::routing::ProfileCreationArgs trace;

  trace.id = A_surfxml_trace_id;
  trace.file = A_surfxml_trace_file;
  trace.periodicity = surf_parse_get_double(A_surfxml_trace_periodicity);
  trace.pc_data = surfxml_pcdata;

  sg_platf_new_trace(&trace);
}

void STag_surfxml_trace___connect()
{
  simgrid::kernel::routing::TraceConnectCreationArgs trace_connect;

  trace_connect.element = A_surfxml_trace___connect_element;
  trace_connect.trace = A_surfxml_trace___connect_trace;

  switch (A_surfxml_trace___connect_kind) {
  case AU_surfxml_trace___connect_kind:
  case A_surfxml_trace___connect_kind_SPEED:
    trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::SPEED;
    break;
  case A_surfxml_trace___connect_kind_BANDWIDTH:
    trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::BANDWIDTH;
    break;
  case A_surfxml_trace___connect_kind_HOST___AVAIL:
    trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::HOST_AVAIL;
    break;
  case A_surfxml_trace___connect_kind_LATENCY:
    trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::LATENCY;
    break;
  case A_surfxml_trace___connect_kind_LINK___AVAIL:
    trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::LINK_AVAIL;
    break;
  default:
    surf_parse_error("Invalid trace kind");
  }
  sg_platf_trace_connect(&trace_connect);
}

void STag_surfxml_AS()
{
  AX_surfxml_zone_id = AX_surfxml_AS_id;
  AX_surfxml_zone_routing = AX_surfxml_AS_routing;
  STag_surfxml_zone();
}

void ETag_surfxml_AS()
{
  ETag_surfxml_zone();
}

void STag_surfxml_zone()
{
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
  simgrid::kernel::routing::ZoneCreationArgs zone;
  zone.id      = A_surfxml_zone_id;
  zone.routing = A_surfxml_zone_routing;
  sg_platf_new_Zone_begin(&zone);
}

void ETag_surfxml_zone()
{
  sg_platf_new_Zone_set_properties(property_sets.back());
  delete property_sets.back();
  property_sets.pop_back();

  sg_platf_new_Zone_seal();
}

void STag_surfxml_config()
{
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
  if (_sg_cfg_init_status == 2) {
    surf_parse_error("All <config> tags must be given before any platform elements (such as <zone>, <host>, <cluster>, "
                     "<link>, etc).");
  }
}

void ETag_surfxml_config()
{
  // Sort config elements before applying.
  // That's a little waste of time, but not doing so would break the tests
  auto current_property_set = property_sets.back();

  std::vector<std::string> keys;
  for (auto const& kv : *current_property_set) {
    keys.push_back(kv.first);
  }
  std::sort(keys.begin(), keys.end());
  for (std::string key : keys) {
    if (simgrid::config::is_default(key.c_str())) {
      std::string cfg = key + ":" + current_property_set->at(key);
      simgrid::config::set_parse(cfg);
    } else
      XBT_INFO("The custom configuration '%s' is already defined by user!", key.c_str());
  }
  XBT_DEBUG("End configuration name = %s",A_surfxml_config_id);

  delete current_property_set;
  property_sets.pop_back();
}

static std::vector<std::string> arguments;

void STag_surfxml_process()
{
  AX_surfxml_actor_function = AX_surfxml_process_function;
  STag_surfxml_actor();
}

void STag_surfxml_actor()
{
  property_sets.push_back(new std::unordered_map<std::string, std::string>());
  arguments.assign(1, A_surfxml_actor_function);
}

void ETag_surfxml_process()
{
  AX_surfxml_actor_host = AX_surfxml_process_host;
  AX_surfxml_actor_function = AX_surfxml_process_function;
  AX_surfxml_actor_start___time = AX_surfxml_process_start___time;
  AX_surfxml_actor_kill___time = AX_surfxml_process_kill___time;
  AX_surfxml_actor_on___failure = (AT_surfxml_actor_on___failure)AX_surfxml_process_on___failure;
  ETag_surfxml_actor();
}

void ETag_surfxml_actor()
{
  simgrid::kernel::routing::ActorCreationArgs actor;

  actor.properties = property_sets.back();
  property_sets.pop_back();

  actor.args.swap(arguments);
  actor.host       = A_surfxml_actor_host;
  actor.function   = A_surfxml_actor_function;
  actor.start_time = surf_parse_get_double(A_surfxml_actor_start___time);
  actor.kill_time  = surf_parse_get_double(A_surfxml_actor_kill___time);

  switch (A_surfxml_actor_on___failure) {
  case AU_surfxml_actor_on___failure:
  case A_surfxml_actor_on___failure_DIE:
    actor.restart_on_failure = false;
    break;
  case A_surfxml_actor_on___failure_RESTART:
    actor.restart_on_failure = true;
    break;
  default:
    surf_parse_error("Invalid on failure behavior");
  }

  sg_platf_new_actor(&actor);
}

void STag_surfxml_argument(){
  arguments.emplace_back(A_surfxml_argument_value);
}

void STag_surfxml_model___prop(){
  if (not current_model_property_set)
    current_model_property_set = new std::unordered_map<std::string, std::string>();

  current_model_property_set->insert({A_surfxml_model___prop_id, A_surfxml_model___prop_value});
}

void ETag_surfxml_prop(){/* Nothing to do */}
void STag_surfxml_random(){/* Nothing to do */}
void ETag_surfxml_random(){/* Nothing to do */}
void ETag_surfxml_trace___connect(){/* Nothing to do */}
void STag_surfxml_trace()
{ /* Nothing to do */
}
void ETag_surfxml_router(){/*Nothing to do*/}
void ETag_surfxml_host___link(){/* Nothing to do */}
void ETag_surfxml_cabinet(){/* Nothing to do */}
void ETag_surfxml_peer(){/* Nothing to do */}
void STag_surfxml_backbone(){/* Nothing to do */}
void ETag_surfxml_link___ctn(){/* Nothing to do */}
void ETag_surfxml_argument(){/* Nothing to do */}
void ETag_surfxml_model___prop(){/* Nothing to do */}

/* Open and Close parse file */
YY_BUFFER_STATE surf_input_buffer;

void surf_parse_open(const std::string& file)
{
  surf_parsed_filename = file;
  std::string dir      = simgrid::xbt::Path(file).get_dir_name();
  surf_path.push_back(dir);

  surf_file_to_parse = surf_fopen(file, "r");
  if (surf_file_to_parse == nullptr)
    throw std::invalid_argument(std::string("Unable to open '") + file + "' from '" + simgrid::xbt::Path().get_name() +
                                "'. Does this file exist?");
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close()
{
  surf_path.pop_back(); // remove the dirname of the opened file, that was added in surf_parse_open()

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
    surf_file_to_parse = nullptr; //Must be reset for Bypass
  }
}

/* Call the lexer to parse the currently opened file */
void surf_parse()
{
  bool err = surf_parse_lex();
  surf_parse_assert(not err, "Flex returned an error code");
}
