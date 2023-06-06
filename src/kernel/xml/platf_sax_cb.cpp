/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <xbt/file.hpp>
#include <xbt/parse_units.hpp>

#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/kernel/xml/platf.hpp"
#include "src/kernel/xml/platf_private.hpp"
#include "src/simgrid/sg_config.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "simgrid_dtd.c"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(platf_parse, ker_platform, "Logging specific to the parsing of platform files");

std::string simgrid_parsed_filename;                            // Currently parsed file (for the error messages)
static std::vector<simgrid::s4u::LinkInRoute> parsed_link_list; /* temporary store of current link list of a route */

static bool fire_on_platform_created_callback;

/* Helping functions */
void simgrid_parse_assert(bool cond, const std::string& msg)
{
  if (not cond)
    simgrid_parse_error(msg);
}

void simgrid_parse_error(const std::string& msg)
{
  throw simgrid::ParseError(simgrid_parsed_filename, simgrid_parse_lineno, msg);
}

void simgrid_parse_assert_netpoint(const std::string& hostname, const std::string& pre, const std::string& post)
{
  if (simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(hostname) != nullptr) // found
    return;

  std::string msg = pre + hostname + post + " Existing netpoints:\n";

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
  simgrid_parse_error(msg);
}

double simgrid_parse_get_double(const std::string& s)
{
  try {
    return std::stod(s);
  } catch (const std::invalid_argument&) {
    simgrid_parse_error(s + " is not a double");
  }
}

int simgrid_parse_get_int(const std::string& s)
{
  try {
    return std::stoi(s);
  } catch (const std::invalid_argument&) {
    simgrid_parse_error(s + " is not an int");
  }
}

/* Turn something like "1-4,6,9-11" into the vector {1,2,3,4,6,9,10,11} */
static void explodesRadical(const std::string& radicals, std::vector<int>* exploded)
{
  // Make all hosts
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, radicals, boost::is_any_of(","));
  for (auto const& group : radical_elements) {
    std::vector<std::string> radical_ends;
    boost::split(radical_ends, group, boost::is_any_of("-"));
    int start = simgrid_parse_get_int(radical_ends.front());
    int end   = 0;

    switch (radical_ends.size()) {
      case 1:
        end = start;
        break;
      case 2:
        end = simgrid_parse_get_int(radical_ends.back());
        break;
      default:
        simgrid_parse_error("Malformed radical: " + group);
    }
    for (int i = start; i <= end; i++)
      exploded->push_back(i);
  }
}

/* Trace related stuff */
XBT_PRIVATE std::unordered_map<std::string, simgrid::kernel::profile::Profile*>
    traces_set_list; // shown to sg_platf.cpp
static std::unordered_map<std::string, std::string> trace_connect_list_host_avail;
static std::unordered_map<std::string, std::string> trace_connect_list_host_speed;
static std::unordered_map<std::string, std::string> trace_connect_list_link_avail;
static std::unordered_map<std::string, std::string> trace_connect_list_link_bw;
static std::unordered_map<std::string, std::string> trace_connect_list_link_lat;

static void sg_platf_trace_connect(simgrid::kernel::routing::TraceConnectCreationArgs* trace_connect)
{
  simgrid_parse_assert(traces_set_list.find(trace_connect->trace) != traces_set_list.end(),
                       "Cannot connect trace " + trace_connect->trace + " to " + trace_connect->element +
                           ": trace unknown");

  switch (trace_connect->kind) {
    case simgrid::kernel::routing::TraceConnectKind::HOST_AVAIL:
      trace_connect_list_host_avail.try_emplace(trace_connect->trace, trace_connect->element);
      break;
    case simgrid::kernel::routing::TraceConnectKind::SPEED:
      trace_connect_list_host_speed.try_emplace(trace_connect->trace, trace_connect->element);
      break;
    case simgrid::kernel::routing::TraceConnectKind::LINK_AVAIL:
      trace_connect_list_link_avail.try_emplace(trace_connect->trace, trace_connect->element);
      break;
    case simgrid::kernel::routing::TraceConnectKind::BANDWIDTH:
      trace_connect_list_link_bw.try_emplace(trace_connect->trace, trace_connect->element);
      break;
    case simgrid::kernel::routing::TraceConnectKind::LATENCY:
      trace_connect_list_link_lat.try_emplace(trace_connect->trace, trace_connect->element);
      break;
    default:
      simgrid_parse_error("Cannot connect trace " + trace_connect->trace + " to " + trace_connect->element +
                          ": unknown kind of trace");
  }
}

/*
 * All the callback lists that can be overridden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */

static std::vector<std::unordered_map<std::string, std::string>> property_sets;

static FILE* file_to_parse = nullptr;

/* Stuff relative to storage */
void STag_simgrid_parse_storage()
{
  xbt_die("<storage> tag was removed in SimGrid v3.27. Please stop using it now.");
}

void ETag_simgrid_parse_storage()
{
  /* Won't happen since <storage> is now removed since v3.27. */
}
void STag_simgrid_parse_storage___type()
{
  xbt_die("<storage_type> tag was removed in SimGrid v3.27. Please stop using it now.");
}
void ETag_simgrid_parse_storage___type()
{
  /* Won't happen since <storage_type> is now removed since v3.27. */
}

void STag_simgrid_parse_mount()
{
  xbt_die("<mount> tag was removed in SimGrid v3.27. Please stop using it now.");
}

void ETag_simgrid_parse_mount()
{
  /* Won't happen since <mount> is now removed since v3.27. */
}

void STag_simgrid_parse_include()
{
  xbt_die("<include> tag was removed in SimGrid v3.18. Please stop using it now.");
}

void ETag_simgrid_parse_include()
{
  /* Won't happen since <include> is now removed since v3.18. */
}

/* Stag and Etag parse functions */
void STag_simgrid_parse_platform()
{
  /* Use fixed point arithmetic to avoid rounding errors ("4.1" for example cannot be represented exactly as a floating
   * point number) */
  const long int version           = lround(100.0 * simgrid_parse_get_double(A_simgrid_parse_platform_version));
  const std::string version_string = std::to_string(version / 100) + "." + std::to_string(version % 100);

  simgrid_parse_assert(version >= 100L, "******* BIG FAT WARNING *********\n "
                                        "You're using an ancient XML file.\n"
                                        "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
                                        "instead of MBytes, MFlops and seconds.\n"

                                        "Use simgrid_update_xml to update your file automatically. "
                                        "This program is installed automatically with SimGrid, or "
                                        "available in the tools/ directory of the source archive.\n"

                                        "Please check also out the SURF section of the ChangeLog for "
                                        "the 3.1 version for more information.");
  simgrid_parse_assert(version >= 300L, "******* BIG FAT WARNING *********\n "
                                        "You're using an old XML file.\n"
                                        "Use simgrid_update_xml to update your file automatically. "
                                        "This program is installed automatically with SimGrid, or "
                                        "available in the tools/ directory of the source archive.");
  simgrid_parse_assert(
      version >= 400L,
      "******* THIS FILE IS TOO OLD (v:" + version_string +
          ") *********\n "
          "Changes introduced in SimGrid 3.13:\n"
          "  - 'power' attribute of hosts (and others) got renamed to 'speed'.\n"
          "  - In <trace_connect>, attribute kind=\"POWER\" is now kind=\"SPEED\".\n"
          "  - DOCTYPE now point to the rignt URL.\n"
          "  - speed, bandwidth and latency attributes now MUST have an explicit unit (f, Bps, s by default)"
          "\n\n"
          "Use simgrid_update_xml to update your file automatically. "
          "This program is installed automatically with SimGrid, or "
          "available in the tools/ directory of the source archive.");
  if (version < 410L) {
    XBT_INFO("You're using a v%s XML file (%s) while the current standard is v4.1 "
             "That's fine, the new version is backward compatible. \n\n"
             "Use simgrid_update_xml to update your file automatically to get rid of this warning. "
             "This program is installed automatically with SimGrid, or "
             "available in the tools/ directory of the source archive.",
             version_string.c_str(), simgrid_parsed_filename.c_str());
  }
  simgrid_parse_assert(version <= 410L,
                       "******* THIS FILE COMES FROM THE FUTURE (v:" + version_string +
                           ") *********\n "
                           "The most recent formalism that this version of SimGrid understands is v4.1.\n"
                           "Please update your code, or use another, more adapted, file.");
}

static void add_remote_disks()
{
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    const char* remote_disk_str = host->get_property("remote_disk");
    if (not remote_disk_str)
      continue;
    std::vector<std::string> tokens;
    boost::split(tokens, remote_disk_str, boost::is_any_of(":"));
    const simgrid::s4u::Host* remote_host = simgrid::s4u::Host::by_name_or_null(tokens[2]);
    xbt_assert(remote_host, "You're trying to access a host that does not exist. Please check your platform file");

    const simgrid::s4u::Disk* disk = nullptr;
    for (auto const& d : remote_host->get_disks())
      if (d->get_name() == tokens[1]) {
        disk = d;
        break;
      }

    xbt_assert(disk, "You're trying to mount a disk that does not exist. Please check your platform file");
    host->add_disk(disk);

    XBT_DEBUG("Host '%s' wants to access a remote disk: %s of %s", host->get_cname(), disk->get_cname(),
              remote_host->get_cname());
    XBT_DEBUG("Host '%s' now has %zu disks", host->get_cname(), host->get_disks().size());
  }
}

static void remove_remote_disks()
{
  XBT_DEBUG("Simulation is over, time to unregister remote disks if any");
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    const char* remote_disk_str = host->get_property("remote_disk");
    if (remote_disk_str) {
      std::vector<std::string> tokens;
      boost::split(tokens, remote_disk_str, boost::is_any_of(":"));
      XBT_DEBUG("Host '%s' wants to unmount a remote disk: %s of %s", host->get_cname(),
                tokens[1].c_str(), tokens[2].c_str());
      host->remove_disk(tokens[1]);
      XBT_DEBUG("Host '%s' now has %zu disks", host->get_cname(), host->get_disks().size());
    }
  }
}

void ETag_simgrid_parse_platform()
{
  simgrid::s4u::Engine::on_platform_created_cb(&add_remote_disks);
  simgrid::s4u::Engine::on_simulation_end_cb(&remove_remote_disks);
  if (fire_on_platform_created_callback)
    simgrid::s4u::Engine::on_platform_created();
}

void STag_simgrid_parse_prop()
{
  property_sets.back().try_emplace(A_simgrid_parse_prop_id, A_simgrid_parse_prop_value);
  XBT_DEBUG("add prop %s=%s into current property set %p", A_simgrid_parse_prop_id, A_simgrid_parse_prop_value,
            &(property_sets.back()));
}

void STag_simgrid_parse_host()
{
  simgrid::kernel::routing::HostCreationArgs host;
  property_sets.emplace_back();
  host.id = A_simgrid_parse_host_id;

  host.speed_per_pstate = xbt_parse_get_all_speeds(simgrid_parsed_filename, simgrid_parse_lineno,
                                                   A_simgrid_parse_host_speed, "speed of host " + host.id);

  XBT_DEBUG("pstate: %s", A_simgrid_parse_host_pstate);
  host.core_amount = simgrid_parse_get_int(A_simgrid_parse_host_core);

  if (A_simgrid_parse_host_availability___file[0] != '\0') {
    XBT_WARN("The availability_file attribute in <host> is now deprecated. Please, use 'speed_file' instead.");
    host.speed_trace = simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_host_availability___file);
  }
  if (A_simgrid_parse_host_speed___file[0] != '\0')
    host.speed_trace = simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_host_speed___file);
  host.state_trace = A_simgrid_parse_host_state___file[0]
                         ? simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_host_state___file)
                         : nullptr;
  host.coord       = A_simgrid_parse_host_coordinates;

  sg_platf_new_host_begin(&host);
}

void ETag_simgrid_parse_host()
{
  sg_platf_new_host_set_properties(property_sets.back());
  property_sets.pop_back();

  sg_platf_new_host_seal(simgrid_parse_get_int(A_simgrid_parse_host_pstate));
}

void STag_simgrid_parse_disk()
{
  property_sets.emplace_back();
}

void ETag_simgrid_parse_disk()
{
  simgrid::kernel::routing::DiskCreationArgs disk;
  disk.properties = property_sets.back();
  property_sets.pop_back();

  disk.id       = A_simgrid_parse_disk_id;
  disk.read_bw  = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_disk_read___bw,
                                          "read_bw of disk " + disk.id);
  disk.write_bw = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno,
                                          A_simgrid_parse_disk_write___bw, "write_bw of disk " + disk.id);

  sg_platf_new_disk(&disk);
}

void STag_simgrid_parse_host___link()
{
  XBT_DEBUG("Create a Host_link for %s", A_simgrid_parse_host___link_id);
  simgrid::kernel::routing::HostLinkCreationArgs host_link;

  host_link.id        = A_simgrid_parse_host___link_id;
  host_link.link_up   = A_simgrid_parse_host___link_up;
  host_link.link_down = A_simgrid_parse_host___link_down;
  sg_platf_new_hostlink(&host_link);
}

void STag_simgrid_parse_router()
{
  sg_platf_new_router(A_simgrid_parse_router_id, A_simgrid_parse_router_coordinates);
}

void ETag_simgrid_parse_cluster()
{
  simgrid::kernel::routing::ClusterCreationArgs cluster;
  cluster.properties = property_sets.back();
  property_sets.pop_back();

  cluster.id     = A_simgrid_parse_cluster_id;
  cluster.prefix = A_simgrid_parse_cluster_prefix;
  cluster.suffix = A_simgrid_parse_cluster_suffix;
  explodesRadical(A_simgrid_parse_cluster_radical, &cluster.radicals);

  cluster.speeds      = xbt_parse_get_all_speeds(simgrid_parsed_filename, simgrid_parse_lineno,
                                                 A_simgrid_parse_cluster_speed, "speed of cluster " + cluster.id);
  cluster.core_amount = simgrid_parse_get_int(A_simgrid_parse_cluster_core);
  cluster.bw  = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_bw,
                                        "bw of cluster " + cluster.id);
  cluster.lat = xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_lat,
                                   "lat of cluster " + cluster.id);
  if (strcmp(A_simgrid_parse_cluster_bb___bw, ""))
    cluster.bb_bw = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno,
                                            A_simgrid_parse_cluster_bb___bw, "bb_bw of cluster " + cluster.id);
  if (strcmp(A_simgrid_parse_cluster_bb___lat, ""))
    cluster.bb_lat = xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_bb___lat,
                                        "bb_lat of cluster " + cluster.id);
  if (strcmp(A_simgrid_parse_cluster_limiter___link, ""))
    cluster.limiter_link =
        xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_limiter___link,
                                "limiter_link of cluster " + cluster.id);
  if (strcmp(A_simgrid_parse_cluster_loopback___bw, ""))
    cluster.loopback_bw =
        xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_loopback___bw,
                                "loopback_bw of cluster " + cluster.id);
  if (strcmp(A_simgrid_parse_cluster_loopback___lat, ""))
    cluster.loopback_lat =
        xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cluster_loopback___lat,
                           "loopback_lat of cluster " + cluster.id);

  switch (AX_simgrid_parse_cluster_topology) {
    case A_simgrid_parse_cluster_topology_FLAT:
      cluster.topology = simgrid::kernel::routing::ClusterTopology::FLAT;
      break;
    case A_simgrid_parse_cluster_topology_TORUS:
      cluster.topology = simgrid::kernel::routing::ClusterTopology::TORUS;
      break;
    case A_simgrid_parse_cluster_topology_FAT___TREE:
      cluster.topology = simgrid::kernel::routing::ClusterTopology::FAT_TREE;
      break;
    case A_simgrid_parse_cluster_topology_DRAGONFLY:
      cluster.topology = simgrid::kernel::routing::ClusterTopology::DRAGONFLY;
      break;
    default:
      simgrid_parse_error("Invalid cluster topology for cluster " + cluster.id);
  }
  cluster.topo_parameters = A_simgrid_parse_cluster_topo___parameters;
  cluster.router_id       = A_simgrid_parse_cluster_router___id;

  switch (AX_simgrid_parse_cluster_sharing___policy) {
    case A_simgrid_parse_cluster_sharing___policy_SHARED:
      cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SHARED;
      break;
    case A_simgrid_parse_cluster_sharing___policy_FULLDUPLEX:
      XBT_WARN("FULLDUPLEX is now deprecated. Please update your platform file to use SPLITDUPLEX instead.");
      cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
      break;
    case A_simgrid_parse_cluster_sharing___policy_SPLITDUPLEX:
      cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
      break;
    case A_simgrid_parse_cluster_sharing___policy_FATPIPE:
      cluster.sharing_policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
      break;
    default:
      simgrid_parse_error("Invalid cluster sharing policy for cluster " + cluster.id);
  }
  switch (AX_simgrid_parse_cluster_bb___sharing___policy) {
    case A_simgrid_parse_cluster_bb___sharing___policy_FATPIPE:
      cluster.bb_sharing_policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
      break;
    case A_simgrid_parse_cluster_bb___sharing___policy_SHARED:
      cluster.bb_sharing_policy = simgrid::s4u::Link::SharingPolicy::SHARED;
      break;
    default:
      simgrid_parse_error("Invalid bb sharing policy in cluster " + cluster.id);
  }

  sg_platf_new_tag_cluster(&cluster);
}

void STag_simgrid_parse_cluster()
{
  property_sets.emplace_back();
}

void STag_simgrid_parse_cabinet()
{
  simgrid::kernel::routing::CabinetCreationArgs cabinet;
  cabinet.id     = A_simgrid_parse_cabinet_id;
  cabinet.prefix = A_simgrid_parse_cabinet_prefix;
  cabinet.suffix = A_simgrid_parse_cabinet_suffix;
  cabinet.speed  = xbt_parse_get_speed(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cabinet_speed,
                                       "speed of cabinet " + cabinet.id);
  cabinet.bw     = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cabinet_bw,
                                           "bw of cabinet " + cabinet.id);
  cabinet.lat    = xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_cabinet_lat,
                                      "lat of cabinet " + cabinet.id);
  explodesRadical(A_simgrid_parse_cabinet_radical, &cabinet.radicals);

  sg_platf_new_cabinet(&cabinet);
}

void STag_simgrid_parse_peer()
{
  simgrid::kernel::routing::PeerCreationArgs peer;

  peer.id     = A_simgrid_parse_peer_id;
  peer.speed  = xbt_parse_get_speed(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_peer_speed,
                                    "speed of peer " + peer.id);
  peer.bw_in  = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_peer_bw___in,
                                        "bw_in of peer " + peer.id);
  peer.bw_out = xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_peer_bw___out,
                                        "bw_out of peer " + peer.id);
  peer.coord  = A_simgrid_parse_peer_coordinates;
  peer.speed_trace = nullptr;
  if (A_simgrid_parse_peer_availability___file[0] != '\0') {
    XBT_WARN("The availability_file attribute in <peer> is now deprecated. Please, use 'speed_file' instead.");
    peer.speed_trace = simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_peer_availability___file);
  }
  if (A_simgrid_parse_peer_speed___file[0] != '\0')
    peer.speed_trace = simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_peer_speed___file);
  peer.state_trace = A_simgrid_parse_peer_state___file[0]
                         ? simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_peer_state___file)
                         : nullptr;

  if (A_simgrid_parse_peer_lat[0] != '\0')
    XBT_WARN("The latency attribute in <peer> is now deprecated. Use the z coordinate instead of '%s'.",
             A_simgrid_parse_peer_lat);

  sg_platf_new_peer(&peer);
}

void STag_simgrid_parse_link()
{
  property_sets.emplace_back();
}

void ETag_simgrid_parse_link()
{
  simgrid::kernel::routing::LinkCreationArgs link;

  link.properties = property_sets.back();
  property_sets.pop_back();

  link.id         = A_simgrid_parse_link_id;
  link.bandwidths = xbt_parse_get_bandwidths(simgrid_parsed_filename, simgrid_parse_lineno,
                                             A_simgrid_parse_link_bandwidth, "bandwidth of link " + link.id);
  link.bandwidth_trace =
      A_simgrid_parse_link_bandwidth___file[0]
          ? simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_link_bandwidth___file)
          : nullptr;
  link.latency       = xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_link_latency,
                                          "latency of link " + link.id);
  link.latency_trace = A_simgrid_parse_link_latency___file[0]
                           ? simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_link_latency___file)
                           : nullptr;
  link.state_trace   = A_simgrid_parse_link_state___file[0]
                           ? simgrid::kernel::profile::ProfileBuilder::from_file(A_simgrid_parse_link_state___file)
                           : nullptr;

  switch (A_simgrid_parse_link_sharing___policy) {
    case A_simgrid_parse_link_sharing___policy_SHARED:
      link.policy = simgrid::s4u::Link::SharingPolicy::SHARED;
      break;
    case A_simgrid_parse_link_sharing___policy_FATPIPE:
      link.policy = simgrid::s4u::Link::SharingPolicy::FATPIPE;
      break;
    case A_simgrid_parse_link_sharing___policy_FULLDUPLEX:
      XBT_WARN("FULLDUPLEX is now deprecated. Please update your platform file to use SPLITDUPLEX instead.");
      link.policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
      break;
    case A_simgrid_parse_link_sharing___policy_SPLITDUPLEX:
      link.policy = simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
      break;
    case A_simgrid_parse_link_sharing___policy_WIFI:
      link.policy = simgrid::s4u::Link::SharingPolicy::WIFI;
      break;
    default:
      simgrid_parse_error("Invalid sharing policy in link " + link.id);
  }

  sg_platf_new_link(&link);
}

void STag_simgrid_parse_link___ctn()
{
  const auto* engine = simgrid::s4u::Engine::get_instance();
  const simgrid::s4u::Link* link;
  simgrid::s4u::LinkInRoute::Direction direction = simgrid::s4u::LinkInRoute::Direction::NONE;
  switch (A_simgrid_parse_link___ctn_direction) {
    case AU_simgrid_parse_link___ctn_direction:
    case A_simgrid_parse_link___ctn_direction_NONE:
      link = engine->link_by_name(A_simgrid_parse_link___ctn_id);
      break;
    case A_simgrid_parse_link___ctn_direction_UP:
      link      = engine->split_duplex_link_by_name(A_simgrid_parse_link___ctn_id);
      direction = simgrid::s4u::LinkInRoute::Direction::UP;
      break;
    case A_simgrid_parse_link___ctn_direction_DOWN:
      link      = engine->split_duplex_link_by_name(A_simgrid_parse_link___ctn_id);
      direction = simgrid::s4u::LinkInRoute::Direction::DOWN;
      break;
    default:
      simgrid_parse_error(std::string("Invalid direction for link ") + A_simgrid_parse_link___ctn_id);
  }

  const char* dirname;
  switch (A_simgrid_parse_link___ctn_direction) {
    case A_simgrid_parse_link___ctn_direction_UP:
      dirname = " (upward)";
      break;
    case A_simgrid_parse_link___ctn_direction_DOWN:
      dirname = " (downward)";
      break;
    default:
      dirname = "";
  }
  simgrid_parse_assert(link != nullptr, std::string("No such link: '") + A_simgrid_parse_link___ctn_id + "'" + dirname);
  parsed_link_list.emplace_back(link, direction);
}

void ETag_simgrid_parse_backbone()
{
  auto link = std::make_unique<simgrid::kernel::routing::LinkCreationArgs>();

  link->id = A_simgrid_parse_backbone_id;
  link->bandwidths.push_back(xbt_parse_get_bandwidth(simgrid_parsed_filename, simgrid_parse_lineno,
                                                     A_simgrid_parse_backbone_bandwidth,
                                                     "bandwidth of backbone " + link->id));
  link->latency = xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, A_simgrid_parse_backbone_latency,
                                     "latency of backbone " + link->id);
  link->policy  = simgrid::s4u::Link::SharingPolicy::SHARED;

  routing_cluster_add_backbone(std::move(link));
}

void STag_simgrid_parse_route()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_route_src, "Route src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_route_dst, "Route dst='", "' does name a node.");
}

void STag_simgrid_parse_ASroute()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_ASroute_src, "ASroute src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_ASroute_dst, "ASroute dst='", "' does name a node.");

  simgrid_parse_assert_netpoint(A_simgrid_parse_ASroute_gw___src, "ASroute gw_src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_ASroute_gw___dst, "ASroute gw_dst='", "' does name a node.");
}
void STag_simgrid_parse_zoneRoute()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_zoneRoute_src, "zoneRoute src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_zoneRoute_dst, "zoneRoute dst='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_zoneRoute_gw___src, "zoneRoute gw_src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_zoneRoute_gw___dst, "zoneRoute gw_dst='", "' does name a node.");
}

void STag_simgrid_parse_bypassRoute()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassRoute_src, "bypassRoute src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassRoute_dst, "bypassRoute dst='", "' does name a node.");
}

void STag_simgrid_parse_bypassASroute()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassASroute_src, "bypassASroute src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassASroute_dst, "bypassASroute dst='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassASroute_gw___src, "bypassASroute gw_src='",
                                "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassASroute_gw___dst, "bypassASroute gw_dst='",
                                "' does name a node.");
}
void STag_simgrid_parse_bypassZoneRoute()
{
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassZoneRoute_src, "bypassZoneRoute src='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassZoneRoute_dst, "bypassZoneRoute dst='", "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassZoneRoute_gw___src, "bypassZoneRoute gw_src='",
                                "' does name a node.");
  simgrid_parse_assert_netpoint(A_simgrid_parse_bypassZoneRoute_gw___dst, "bypassZoneRoute gw_dst='",
                                "' does name a node.");
}

void ETag_simgrid_parse_route()
{
  simgrid::kernel::routing::RouteCreationArgs route;

  route.src         = sg_netpoint_by_name_or_null(A_simgrid_parse_route_src); // tested to not be nullptr in start tag
  route.dst         = sg_netpoint_by_name_or_null(A_simgrid_parse_route_dst); // tested to not be nullptr in start tag
  route.symmetrical = (A_simgrid_parse_route_symmetrical == AU_simgrid_parse_route_symmetrical ||
                       A_simgrid_parse_route_symmetrical == A_simgrid_parse_route_symmetrical_YES ||
                       A_simgrid_parse_route_symmetrical == A_simgrid_parse_route_symmetrical_yes);

  route.link_list.swap(parsed_link_list);

  sg_platf_new_route(&route);
}

void ETag_simgrid_parse_ASroute()
{
  AX_simgrid_parse_zoneRoute_src         = AX_simgrid_parse_ASroute_src;
  AX_simgrid_parse_zoneRoute_dst         = AX_simgrid_parse_ASroute_dst;
  AX_simgrid_parse_zoneRoute_gw___src    = AX_simgrid_parse_ASroute_gw___src;
  AX_simgrid_parse_zoneRoute_gw___dst    = AX_simgrid_parse_ASroute_gw___dst;
  AX_simgrid_parse_zoneRoute_symmetrical = (AT_simgrid_parse_zoneRoute_symmetrical)AX_simgrid_parse_ASroute_symmetrical;
  ETag_simgrid_parse_zoneRoute();
}
void ETag_simgrid_parse_zoneRoute()
{
  simgrid::kernel::routing::RouteCreationArgs ASroute;

  ASroute.src = sg_netpoint_by_name_or_null(A_simgrid_parse_zoneRoute_src); // tested to not be nullptr in start tag
  ASroute.dst = sg_netpoint_by_name_or_null(A_simgrid_parse_zoneRoute_dst); // tested to not be nullptr in start tag

  ASroute.gw_src =
      sg_netpoint_by_name_or_null(A_simgrid_parse_zoneRoute_gw___src); // tested to not be nullptr in start tag
  ASroute.gw_dst =
      sg_netpoint_by_name_or_null(A_simgrid_parse_zoneRoute_gw___dst); // tested to not be nullptr in start tag

  ASroute.link_list.swap(parsed_link_list);

  ASroute.symmetrical = (A_simgrid_parse_zoneRoute_symmetrical == AU_simgrid_parse_zoneRoute_symmetrical ||
                         A_simgrid_parse_zoneRoute_symmetrical == A_simgrid_parse_zoneRoute_symmetrical_YES ||
                         A_simgrid_parse_zoneRoute_symmetrical == A_simgrid_parse_zoneRoute_symmetrical_yes);

  sg_platf_new_route(&ASroute);
}

void ETag_simgrid_parse_bypassRoute()
{
  simgrid::kernel::routing::RouteCreationArgs route;

  route.src = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassRoute_src); // tested to not be nullptr in start tag
  route.dst = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassRoute_dst); // tested to not be nullptr in start tag
  route.symmetrical = false;

  route.link_list.swap(parsed_link_list);

  sg_platf_new_bypass_route(&route);
}

void ETag_simgrid_parse_bypassASroute()
{
  AX_simgrid_parse_bypassZoneRoute_src      = AX_simgrid_parse_bypassASroute_src;
  AX_simgrid_parse_bypassZoneRoute_dst      = AX_simgrid_parse_bypassASroute_dst;
  AX_simgrid_parse_bypassZoneRoute_gw___src = AX_simgrid_parse_bypassASroute_gw___src;
  AX_simgrid_parse_bypassZoneRoute_gw___dst = AX_simgrid_parse_bypassASroute_gw___dst;
  ETag_simgrid_parse_bypassZoneRoute();
}
void ETag_simgrid_parse_bypassZoneRoute()
{
  simgrid::kernel::routing::RouteCreationArgs ASroute;

  ASroute.src = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassZoneRoute_src);
  ASroute.dst = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassZoneRoute_dst);
  ASroute.link_list.swap(parsed_link_list);

  ASroute.symmetrical = false;

  ASroute.gw_src = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassZoneRoute_gw___src);
  ASroute.gw_dst = sg_netpoint_by_name_or_null(A_simgrid_parse_bypassZoneRoute_gw___dst);

  sg_platf_new_bypass_route(&ASroute);
}

void ETag_simgrid_parse_trace()
{
  simgrid::kernel::routing::ProfileCreationArgs trace;

  trace.id          = A_simgrid_parse_trace_id;
  trace.file        = A_simgrid_parse_trace_file;
  trace.periodicity = simgrid_parse_get_double(A_simgrid_parse_trace_periodicity);
  trace.pc_data     = simgrid_parse_pcdata;

  sg_platf_new_trace(&trace);
}

void STag_simgrid_parse_trace___connect()
{
  simgrid::kernel::routing::TraceConnectCreationArgs trace_connect;

  trace_connect.element = A_simgrid_parse_trace___connect_element;
  trace_connect.trace   = A_simgrid_parse_trace___connect_trace;

  switch (A_simgrid_parse_trace___connect_kind) {
    case AU_simgrid_parse_trace___connect_kind:
    case A_simgrid_parse_trace___connect_kind_SPEED:
      trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::SPEED;
      break;
    case A_simgrid_parse_trace___connect_kind_BANDWIDTH:
      trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::BANDWIDTH;
      break;
    case A_simgrid_parse_trace___connect_kind_HOST___AVAIL:
      trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::HOST_AVAIL;
      break;
    case A_simgrid_parse_trace___connect_kind_LATENCY:
      trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::LATENCY;
      break;
    case A_simgrid_parse_trace___connect_kind_LINK___AVAIL:
      trace_connect.kind = simgrid::kernel::routing::TraceConnectKind::LINK_AVAIL;
      break;
    default:
      simgrid_parse_error("Invalid trace kind");
  }
  sg_platf_trace_connect(&trace_connect);
}

void STag_simgrid_parse_AS()
{
  AX_simgrid_parse_zone_id      = AX_simgrid_parse_AS_id;
  AX_simgrid_parse_zone_routing = AX_simgrid_parse_AS_routing;
  STag_simgrid_parse_zone();
}

void ETag_simgrid_parse_AS()
{
  ETag_simgrid_parse_zone();
}

void STag_simgrid_parse_zone()
{
  property_sets.emplace_back();
  simgrid::kernel::routing::ZoneCreationArgs zone;
  zone.id      = A_simgrid_parse_zone_id;
  zone.routing = A_simgrid_parse_zone_routing;
  sg_platf_new_zone_begin(&zone);
}

void ETag_simgrid_parse_zone()
{
  sg_platf_new_zone_set_properties(property_sets.back());
  property_sets.pop_back();
  sg_platf_new_zone_seal();
}

void STag_simgrid_parse_config()
{
  property_sets.emplace_back();
  XBT_DEBUG("START configuration name = %s", A_simgrid_parse_config_id);
  if (_sg_cfg_init_status == 2) {
    simgrid_parse_error(
        "All <config> tags must be given before any platform elements (such as <zone>, <host>, <cluster>, "
        "<link>, etc).");
  }
}

void ETag_simgrid_parse_config()
{
  // Sort config elements before applying.
  // That's a little waste of time, but not doing so would break the tests
  auto current_property_set = property_sets.back();

  std::vector<std::string> keys;
  for (auto const& [key, _] : current_property_set) {
    keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());
  for (const std::string& key : keys) {
    if (simgrid::config::is_default(key.c_str())) {
      std::string cfg = key + ":" + current_property_set.at(key);
      simgrid::config::set_parse(cfg);
    } else
      XBT_INFO("The custom configuration '%s' is already defined by user!", key.c_str());
  }
  XBT_DEBUG("End configuration name = %s", A_simgrid_parse_config_id);

  property_sets.pop_back();
}

static std::vector<std::string> arguments;

void STag_simgrid_parse_process()
{
  AX_simgrid_parse_actor_function = AX_simgrid_parse_process_function;
  STag_simgrid_parse_actor();
}

void STag_simgrid_parse_actor()
{
  property_sets.emplace_back();
  arguments.assign(1, A_simgrid_parse_actor_function);
}

void ETag_simgrid_parse_process()
{
  AX_simgrid_parse_actor_host         = AX_simgrid_parse_process_host;
  AX_simgrid_parse_actor_function     = AX_simgrid_parse_process_function;
  AX_simgrid_parse_actor_start___time = AX_simgrid_parse_process_start___time;
  AX_simgrid_parse_actor_kill___time  = AX_simgrid_parse_process_kill___time;
  AX_simgrid_parse_actor_on___failure = (AT_simgrid_parse_actor_on___failure)AX_simgrid_parse_process_on___failure;
  ETag_simgrid_parse_actor();
}

void ETag_simgrid_parse_actor()
{
  simgrid::kernel::routing::ActorCreationArgs actor;

  actor.properties = property_sets.back();
  property_sets.pop_back();

  actor.args.swap(arguments);
  actor.host       = A_simgrid_parse_actor_host;
  actor.function   = A_simgrid_parse_actor_function;
  actor.start_time = simgrid_parse_get_double(A_simgrid_parse_actor_start___time);
  actor.kill_time  = simgrid_parse_get_double(A_simgrid_parse_actor_kill___time);

  switch (A_simgrid_parse_actor_on___failure) {
    case AU_simgrid_parse_actor_on___failure:
    case A_simgrid_parse_actor_on___failure_DIE:
      actor.restart_on_failure = false;
      break;
    case A_simgrid_parse_actor_on___failure_RESTART:
      actor.restart_on_failure = true;
      break;
    default:
      simgrid_parse_error("Invalid on failure behavior");
  }

  sg_platf_new_actor(&actor);
}

void STag_simgrid_parse_argument()
{
  arguments.emplace_back(A_simgrid_parse_argument_value);
}

void STag_simgrid_parse_model___prop()
{
  XBT_INFO("Deprecated tag <model_prop> ignored");
}

void ETag_simgrid_parse_prop()
{ /* Nothing to do */
}
void STag_simgrid_parse_random()
{ /* Nothing to do */
}
void ETag_simgrid_parse_random()
{ /* Nothing to do */
}
void ETag_simgrid_parse_trace___connect()
{ /* Nothing to do */
}
void STag_simgrid_parse_trace()
{ /* Nothing to do */
}
void ETag_simgrid_parse_router()
{ /*Nothing to do*/
}
void ETag_simgrid_parse_host___link()
{ /* Nothing to do */
}
void ETag_simgrid_parse_cabinet()
{ /* Nothing to do */
}
void ETag_simgrid_parse_peer()
{ /* Nothing to do */
}
void STag_simgrid_parse_backbone()
{ /* Nothing to do */
}
void ETag_simgrid_parse_link___ctn()
{ /* Nothing to do */
}
void ETag_simgrid_parse_argument()
{ /* Nothing to do */
}
void ETag_simgrid_parse_model___prop()
{ /* Nothing to do */
}

/* Open and Close parse file */
static YY_BUFFER_STATE input_buffer;

void simgrid_parse_open(const std::string& file)
{
  simgrid_parsed_filename = file;
  std::string dir         = simgrid::xbt::Path(file).get_dir_name();
  simgrid::xbt::path_push(dir);

  file_to_parse = simgrid::xbt::path_fopen(file, "r");
  if (file_to_parse == nullptr)
    throw std::invalid_argument("Unable to open '" + file + "' from '" + simgrid::xbt::Path().get_name() +
                                "'. Does this file exist?");
  input_buffer = simgrid_parse__create_buffer(file_to_parse, YY_BUF_SIZE);
  simgrid_parse__switch_to_buffer(input_buffer);
  simgrid_parse_lineno = 1;
}

void simgrid_parse_close()
{
  simgrid::xbt::path_pop(); // remove the dirname of the opened file, that was added in simgrid_parse_open()

  if (file_to_parse) {
    simgrid_parse__delete_buffer(input_buffer);
    fclose(file_to_parse);
    file_to_parse = nullptr; // Must be reset for Bypass
  }
}

/* Call the lexer to parse the currently opened file */
void simgrid_parse(bool fire_on_platform_created_callback_param)
{
  fire_on_platform_created_callback = fire_on_platform_created_callback_param;
  bool err = simgrid_parse_lex();
  simgrid_parse_assert(not err, "Flex returned an error code");

  /* Actually connect the traces now that every elements are created */
  const auto* engine = simgrid::s4u::Engine::get_instance();

  for (auto const& [trace, name] : trace_connect_list_host_avail) {
    simgrid_parse_assert(traces_set_list.find(trace) != traces_set_list.end(),
                         "<trace_connect kind=\"HOST_AVAIL\">: Trace " + trace + " undefined.");
    auto* profile = traces_set_list.at(trace);

    auto* host = engine->host_by_name_or_null(name);
    simgrid_parse_assert(host, "<trace_connect kind=\"HOST_AVAIL\">: Host " + name + " undefined.");
    host->set_state_profile(profile);
  }
  trace_connect_list_host_avail.clear();

  for (auto const& [trace, name] : trace_connect_list_host_speed) {
    simgrid_parse_assert(traces_set_list.find(trace) != traces_set_list.end(),
                         "<trace_connect kind=\"SPEED\">: Trace " + trace + " undefined.");
    auto* profile = traces_set_list.at(trace);

    auto* host = engine->host_by_name_or_null(name);
    simgrid_parse_assert(host, "<trace_connect kind=\"SPEED\">: Host " + name + " undefined.");
    host->set_speed_profile(profile);
  }
  trace_connect_list_host_speed.clear();

  for (auto const& [trace, name] : trace_connect_list_link_avail) {
    simgrid_parse_assert(traces_set_list.find(trace) != traces_set_list.end(),
                         "<trace_connect kind=\"LINK_AVAIL\">: Trace " + trace + " undefined.");
    auto* profile = traces_set_list.at(trace);

    auto* link = engine->link_by_name_or_null(name);
    simgrid_parse_assert(link, "<trace_connect kind=\"LINK_AVAIL\">: Link " + name + " undefined.");
    link->set_state_profile(profile);
  }
  trace_connect_list_link_avail.clear();

  for (auto const& [trace, name] : trace_connect_list_link_bw) {
    simgrid_parse_assert(traces_set_list.find(trace) != traces_set_list.end(),
                         "<trace_connect kind=\"BANDWIDTH\">: Trace " + trace + " undefined.");
    auto* profile = traces_set_list.at(trace);

    auto* link = engine->link_by_name_or_null(name);
    simgrid_parse_assert(link, "<trace_connect kind=\"BANDWIDTH\">: Link " + name + " undefined.");
    link->set_bandwidth_profile(profile);
  }
  trace_connect_list_link_bw.clear();

  for (auto const& [trace, name] : trace_connect_list_link_lat) {
    simgrid_parse_assert(traces_set_list.find(trace) != traces_set_list.end(),
                         "<trace_connect kind=\"LATENCY\">: Trace " + trace + " undefined.");
    auto* profile = traces_set_list.at(trace);

    auto* link = engine->link_by_name_or_null(name);
    simgrid_parse_assert(link, "<trace_connect kind=\"LATENCY\">: Link " + name + " undefined.");
    link->set_latency_profile(profile);
  }
  trace_connect_list_link_lat.clear();
}
