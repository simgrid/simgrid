/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/ns3.hpp"

#include <random>
#include <string>
#include <unordered_set>

#include "src/simgrid/math_utils.h"
#include "src/simgrid/module.hpp"
#include "xbt/config.hpp"
#include "xbt/str.h"
#include "xbt/string.hpp"
#include "xbt/utility.hpp"

#include <ns3/application-container.h>
#include <ns3/core-module.h>
#include <ns3/csma-helper.h>
#include <ns3/event-id.h>
#include <ns3/global-route-manager.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/point-to-point-helper.h>

#include <ns3/mobility-module.h>
#include <ns3/wifi-module.h>

#include "network_ns3.hpp"
#include "src/kernel/resource/models/ns3/ns3_simulator.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/kernel/routing/WifiZone.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the signals
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/xml/platf_private.hpp" // ClusterCreationArgs

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_ns3, res_network, "Network model based on ns-3");

/*****************
 * Crude globals *
 *****************/

extern std::map<std::string, SgFlow*, std::less<>> flow_from_sock;

static int number_of_links    = 1;
static int number_of_networks = 1;

simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> NetPointNs3::EXTENSION_ID;

static std::string transformIpv4Address(ns3::Ipv4Address from)
{
  std::stringstream sstream;
  sstream << from;
  return sstream.str();
}

NetPointNs3::NetPointNs3()
{
  static ns3::InternetStackHelper stack;
  stack.Install(ns3_node_);
}

static void resumeWifiDevice(ns3::Ptr<ns3::WifiNetDevice> device)
{
  device->GetPhy()->ResumeFromOff();
}

/*************
 * Callbacks *
 *************/

static void zoneCreation_cb(simgrid::s4u::NetZone const& zone)
{
  auto const* wifizone = dynamic_cast<simgrid::kernel::routing::WifiZone*>(zone.get_impl());
  if (wifizone == nullptr)
    return;

  /* wifi globals */
  static ns3::WifiHelper wifi;
#if NS3_MINOR_VERSION < 33
  static ns3::YansWifiPhyHelper wifiPhy = ns3::YansWifiPhyHelper::Default();
#else
  static ns3::YansWifiPhyHelper wifiPhy;
#endif
  static ns3::YansWifiChannelHelper wifiChannel = ns3::YansWifiChannelHelper::Default();
  static ns3::WifiMacHelper wifiMac;
  static ns3::MobilityHelper mobility;

#if NS3_MINOR_VERSION < 32
  wifi.SetStandard(ns3::WIFI_PHY_STANDARD_80211n_5GHZ);
#elif NS3_MINOR_VERSION < 36
  wifi.SetStandard(ns3::WIFI_STANDARD_80211n_5GHZ);
#else
  wifi.SetStandard(ns3::WIFI_STANDARD_80211n);
  wifiPhy.Set("ChannelSettings", ns3::StringValue("{0, 0, BAND_5GHZ, 0}"));
#endif

  std::string ssid = wifizone->get_name();
  const char* mcs  = wifizone->get_property("mcs");
  const char* nss  = wifizone->get_property("nss");
  int mcs_value    = mcs ? atoi(mcs) : 3;
  int nss_value    = nss ? atoi(nss) : 1;
#if NS3_MINOR_VERSION < 30
  xbt_assert(nss_value == 1 + (mcs_value / 8),
             "On NS3 < 3.30, NSS value has to satisfy NSS == 1+(MCS/8) constraint. Bailing out");
#endif
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "ControlMode", ns3::StringValue("HtMcs0"), "DataMode",
                               ns3::StringValue("HtMcs" + std::to_string(mcs_value)));
  wifiPhy.SetChannel(wifiChannel.Create());
  wifiPhy.Set("Antennas", ns3::UintegerValue(nss_value));
  wifiPhy.Set("MaxSupportedTxSpatialStreams", ns3::UintegerValue(nss_value));
  wifiPhy.Set("MaxSupportedRxSpatialStreams", ns3::UintegerValue(nss_value));
#if NS3_MINOR_VERSION < 33
  // This fails with "The channel width does not uniquely identify an operating channel" on v3.34,
  // so we specified the ChannelWidth of wifiPhy to 40, above, when creating wifiPhy with v3.34 and higher
  ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(40));
#elif NS3_MINOR_VERSION < 36
  wifiPhy.Set("ChannelWidth", ns3::UintegerValue(40));
#else
  wifiPhy.Set("ChannelSettings", ns3::StringValue("{0, 40, BAND_UNSPECIFIED, 0}"));
#endif
  wifiMac.SetType("ns3::ApWifiMac", "Ssid", ns3::SsidValue(ssid));

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  ns3::Ptr<ns3::ListPositionAllocator> positionAllocS = ns3::CreateObject<ns3::ListPositionAllocator>();
  positionAllocS->Add(ns3::Vector(0, 0, 255 * 100 * number_of_networks + 100 * number_of_links));

  ns3::NetDeviceContainer netDevices;
  NetPointNs3* access_point_netpoint_ns3 = wifizone->get_access_point()->extension<NetPointNs3>();

  ns3::Ptr<ns3::Node> access_point_ns3_node = access_point_netpoint_ns3->ns3_node_;
  ns3::NodeContainer nodes                  = {access_point_ns3_node};
  std::vector<NetPointNs3*> hosts_netpoints = {access_point_netpoint_ns3};
  netDevices.Add(wifi.Install(wifiPhy, wifiMac, access_point_ns3_node));

  wifiMac.SetType("ns3::StaWifiMac", "Ssid", ns3::SsidValue(ssid), "ActiveProbing", ns3::BooleanValue(false));

  NetPointNs3* station_netpoint_ns3    = nullptr;
  ns3::Ptr<ns3::Node> station_ns3_node = nullptr;
  double distance;
  double angle     = 0;
  auto nb_stations = static_cast<double>(wifizone->get_all_hosts().size() - 1);
  double step      = 2 * M_PI / nb_stations;
  for (const auto* station_host : wifizone->get_all_hosts()) {
    station_netpoint_ns3 = station_host->get_netpoint()->extension<NetPointNs3>();
    if (station_netpoint_ns3 == access_point_netpoint_ns3)
      continue;
    hosts_netpoints.push_back(station_netpoint_ns3);
    distance = station_host->get_property("wifi_distance") ? atof(station_host->get_property("wifi_distance")) : 10.0;
    positionAllocS->Add(ns3::Vector(distance * std::cos(angle), distance * std::sin(angle),
                                    255 * 100 * number_of_networks + 100 * number_of_links));
    angle += step;
    station_ns3_node = station_netpoint_ns3->ns3_node_;
    nodes.Add(station_ns3_node);
    netDevices.Add(wifi.Install(wifiPhy, wifiMac, station_ns3_node));
  }

  const char* start_time = wifizone->get_property("start_time");
  int start_time_value   = start_time ? atoi(start_time) : 0;
  for (uint32_t i = 0; i < netDevices.GetN(); i++) {
    ns3::Ptr<ns3::WifiNetDevice> device = ns3::StaticCast<ns3::WifiNetDevice>(netDevices.Get(i));
    device->GetPhy()->SetOffMode();
    ns3::Simulator::Schedule(ns3::Seconds(start_time_value), &resumeWifiDevice, device);
  }

  mobility.SetPositionAllocator(positionAllocS);
  mobility.Install(nodes);
  ns3::Ipv4AddressHelper address;
  std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
  address.SetBase(addr.c_str(), "255.255.0.0");
  XBT_DEBUG("\tInterface stack '%s'", addr.c_str());
  ns3::Ipv4InterfaceContainer addresses = address.Assign(netDevices);
  for (unsigned int i = 0; i < hosts_netpoints.size(); i++) {
    hosts_netpoints[i]->ipv4_address_ = transformIpv4Address(addresses.GetAddress(i));
  }

  if (number_of_links == 255) {
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  } else {
    number_of_links++;
  }
  /* in theory we can compute the routing table only only once at the platform seal
   *  however put it here since or platform_created signal is called before the seal right now */
  ns3::Ipv4GlobalRoutingHelper::RecomputeRoutingTables();
}

static void clusterCreation_cb(simgrid::kernel::routing::ClusterCreationArgs const& cluster)
{
  ns3::NodeContainer Nodes;

  xbt_assert(cluster.topology == simgrid::kernel::routing::ClusterTopology::FLAT,
             "NS-3 is supported only by flat clusters. Do not use with other topologies");

  for (int const& i : cluster.radicals) {
    // Create private link
    std::string host_id = cluster.prefix + std::to_string(i) + cluster.suffix;
    auto const* src     = simgrid::s4u::Host::by_name(host_id)->get_netpoint();
    auto const* dst     = simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(cluster.router_id);
    xbt_assert(dst != nullptr, "No router named %s", cluster.router_id.c_str());

    ns3_add_direct_route(src, dst, cluster.bw, cluster.lat, cluster.sharing_policy); // Any ns-3 route is symmetrical

    // Also add the host to the list of hosts that will be connected to the backbone
    Nodes.Add(src->extension<NetPointNs3>()->ns3_node_);
  }

  // Create link backbone

  xbt_assert(Nodes.GetN() <= 65000, "Cluster with ns-3 is limited to 65000 nodes");
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute("DataRate",
                           ns3::DataRateValue(ns3::DataRate(
                               static_cast<uint64_t>(cluster.bb_bw * 8)))); // ns-3 takes bps, but we provide Bps
  csma.SetChannelAttribute("Delay", ns3::TimeValue(ns3::Seconds(cluster.bb_lat)));
  ns3::NetDeviceContainer devices = csma.Install(Nodes);
  XBT_DEBUG("Create CSMA");

  std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
  XBT_DEBUG("Assign IP Addresses %s to CSMA.", addr.c_str());
  ns3::Ipv4AddressHelper ipv4;
  ipv4.SetBase(addr.c_str(), "255.255.0.0");
  ipv4.Assign(devices);

  if (number_of_links == 255) {
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  } else {
    number_of_links++;
  }
}

static void routeCreation_cb(bool symmetrical, const simgrid::kernel::routing::NetPoint* src,
                             const simgrid::kernel::routing::NetPoint* dst,
                             const simgrid::kernel::routing::NetPoint* /*gw_src*/,
                             const simgrid::kernel::routing::NetPoint* /*gw_dst*/,
                             std::vector<simgrid::kernel::resource::StandardLinkImpl*> const& link_list)
{
  /* ignoring routes from StarZone, not supported */
  if (not src || not dst)
    return;

  if (link_list.size() == 1) {
    auto const* link = static_cast<simgrid::kernel::resource::LinkNS3*>(link_list[0]);

    XBT_DEBUG("Route from '%s' to '%s' with link '%s' %s %s", src->get_cname(), dst->get_cname(), link->get_cname(),
              (link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI ? "(wifi)" : "(wired)"),
              (symmetrical ? "(symmetrical)" : "(not symmetrical)"));

    XBT_DEBUG("\tLink (%s) bw:%fbps lat:%fs", link->get_cname(), link->get_bandwidth(), link->get_latency());

    ns3_add_direct_route(src, dst, link->get_bandwidth(), link->get_latency(), link->get_sharing_policy());
  } else if (static bool warned_about_long_routes = false; not warned_about_long_routes) {
    XBT_WARN("Ignoring a route between %s and %s of length %zu: Only routes of length 1 are considered with ns-3.\n"
             "WARNING: You can ignore this warning if your hosts can still communicate when only considering routes "
             "of length 1.\n"
             "WARNING: Remove long routes to avoid this harmless message; subsequent long routes will be silently "
             "ignored.",
             src->get_cname(), dst->get_cname(), link_list.size());
    warned_about_long_routes = true;
  }
}

/*********
 * Model *
 *********/
// We can't use SIMGRID_REGISTER_NETWORK_MODEL here because ns-3 has a dash in its name
static void XBT_ATTRIB_CONSTRUCTOR(800) simgrid_ns3_network_model_register()
{
  simgrid_network_models().add(
      "ns-3", "Network pseudo-model using the real ns-3 simulator instead of an analytic model", []() {
        auto net_model = std::make_shared<simgrid::kernel::resource::NetworkNS3Model>("NS3 network model");
        auto* engine   = simgrid::kernel::EngineImpl::get_instance();
        engine->add_model(net_model);
        engine->get_netzone_root()->set_network_model(net_model);
      });
}

static simgrid::config::Flag<std::string> ns3_network_model_name("ns3/NetworkModel", {"ns3/TcpModel"},
                                                                 "The ns-3 tcp model can be: NewReno or Cubic",
                                                                 "default", [](const std::string&) {});
static simgrid::config::Flag<std::string> ns3_seed(
    "ns3/seed",
    "The random seed provided to ns-3. Either 'time' to seed with time(), blank to not set (default), or a number.", "",
    [](const std::string& val) {
      if (val.length() == 0)
        return;
      if (strcasecmp(val.c_str(), "time") == 0) {
        std::default_random_engine prng(time(nullptr));
        ns3::RngSeedManager::SetSeed(static_cast<uint32_t>(prng()));
        ns3::RngSeedManager::SetRun(static_cast<uint64_t>(prng()));
      } else {
        auto v = static_cast<int>(xbt_str_parse_int(
            val.c_str(), "Invalid value for option ns3/seed. It must be either 'time', a number, or left empty."));
        ns3::RngSeedManager::SetSeed(v);
        ns3::RngSeedManager::SetRun(v);
      }
    });

namespace simgrid {
namespace kernel::resource {

NetworkNS3Model::NetworkNS3Model(const std::string& name) : NetworkModel(name)
{
  xbt_assert(not sg_link_energy_is_inited(),
             "LinkEnergy plugin and ns-3 network models are not compatible. Are you looking for Ecofen, maybe?");

  NetPointNs3::EXTENSION_ID = routing::NetPoint::extension_create<NetPointNs3>();
  auto const& NetworkProtocol = ns3_network_model_name.get();

  if (NetworkProtocol == "UDP") {
    /*UdpClient=0
UdpEchoClientApplication=0
UdpEchoServerApplication=0
UdpL4Protocol=0
UdpServer=0
UdpSocket=0
UdpSocketImpl=0
UdpTraceClient=0*/
    LogComponentEnable("UdpSocket", ns3::LOG_LEVEL_DEBUG);
    LogComponentEnable("UdpL4Protocol", ns3::LOG_LEVEL_DEBUG);
  } else {
    ns3::Config::SetDefault("ns3::TcpSocket::SegmentSize", ns3::UintegerValue(1000));
    ns3::Config::SetDefault("ns3::TcpSocket::DelAckCount", ns3::UintegerValue(1));
    ns3::Config::SetDefault("ns3::TcpSocketBase::Timestamp", ns3::BooleanValue(false));
  }

  if (NetworkProtocol == "NewReno" || NetworkProtocol == "Cubic") {
    XBT_INFO("Switching Tcp protocol to '%s'", NetworkProtocol.c_str());
    ns3::Config::SetDefault("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::Tcp" + NetworkProtocol));

  } else if (NetworkProtocol == "UDP") {
    XBT_INFO("Switching network protocol to UDP.");

  } else if (NetworkProtocol != "default") {
    xbt_die("The ns3/NetworkModel must be: NewReno, Cubic or UDP but it's '%s'", NetworkProtocol.c_str());
  }

  routing::NetPoint::on_creation.connect([](routing::NetPoint& pt) {
    pt.extension_set<NetPointNs3>(new NetPointNs3());
    XBT_VERB("Declare SimGrid's %s within ns-3", pt.get_cname());
  });

  s4u::Engine::on_platform_created_cb([]() {
    /* Create the ns3 topology based on routing strategy */
    ns3::GlobalRouteManager::BuildGlobalRoutingDatabase();
    ns3::GlobalRouteManager::InitializeRoutes();
  });
  routing::on_cluster_creation.connect(&clusterCreation_cb);
  routing::NetZoneImpl::on_route_creation.connect(&routeCreation_cb);
  s4u::NetZone::on_seal_cb(&zoneCreation_cb);
}

NetworkNS3Model::~NetworkNS3Model()
{
  ns3::Simulator::Destroy();
}

StandardLinkImpl* NetworkNS3Model::create_link(const std::string& name, const std::vector<double>& bandwidths,
                                               routing::NetZoneImpl* englobing_zone)
{
  xbt_assert(bandwidths.size() == 1, "ns-3 links must use only 1 bandwidth.");
  auto* link = new LinkNS3(name, bandwidths[0], s4u::Link::SharingPolicy::SHARED, englobing_zone);
  link->set_model(this);
  return link;
}

StandardLinkImpl* NetworkNS3Model::create_wifi_link(const std::string& name, const std::vector<double>& bandwidths,
                                                    routing::NetZoneImpl* englobing_zone)
{
  xbt_assert(bandwidths.size() == 1, "ns-3 links must use only 1 bandwidth.");
  auto* link = new LinkNS3(name, bandwidths[0], s4u::Link::SharingPolicy::WIFI, englobing_zone);
  link->set_sharing_policy(s4u::Link::SharingPolicy::WIFI, {});
  return link;
}

Action* NetworkNS3Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool /*streamed*/)
{
  xbt_assert(rate == -1,
             "Communication over ns-3 links cannot specify a specific rate. Please use -1 as a value instead of %f.",
             rate);
  return new NetworkNS3Action(this, size, src, dst);
}

/* NS3 is only idempotent with the appropriate patch */
bool NetworkNS3Model::next_occurring_event_is_idempotent()
{
  return false;
}

double NetworkNS3Model::next_occurring_event(double now)
{
  double time_to_next_flow_completion = 0.0;
  XBT_DEBUG("ns3_next_occurring_event");

  // get the first relevant value from the running_actions list

  // If there is no comms in NS-3, then we do not move it forward.
  // We will synchronize NS-3 with SimGrid when starting a new communication.
  // (see NetworkNS3Action::NetworkNS3Action() for more details on this point)
  if (get_started_action_set()->empty() || now == 0.0)
    return -1.0;

  XBT_DEBUG("doing a ns3 simulation for a duration of %f", now);
  ns3_simulator(now);
  time_to_next_flow_completion = ns3::Simulator::Now().GetSeconds() - EngineImpl::get_clock();
  // NS-3 stops as soon as a flow ends,
  // but it does not process the other flows that may finish at the same (simulated) time.
  // If another flow ends at the same time, time_to_next_flow_completion = 0
  if (double_equals(time_to_next_flow_completion, 0, sg_precision_timing))
    time_to_next_flow_completion = 0.0;

  XBT_DEBUG("min         : %f", now);
  XBT_DEBUG("ns-3 time   : %f", ns3::Simulator::Now().GetSeconds());
  XBT_DEBUG("simgrid time: %f", EngineImpl::get_clock());
  XBT_DEBUG("Next completion %f :", time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

void NetworkNS3Model::update_actions_state(double now, double delta)
{
  static std::vector<std::string> socket_to_destroy;

  for (const auto& [ns3_socket, sgFlow] : flow_from_sock) {
    NetworkNS3Action* action = sgFlow->action_;
    XBT_DEBUG("Processing flow %p (socket %s, action %p)", sgFlow, ns3_socket.c_str(), action);
    // Because NS3 stops as soon as a flow is finished, the other flows that ends at the same time may remains in an
    // inconsistent state (i.e. remains_ == 0 but finished_ == false).
    // However, SimGrid considers sometimes that an action with remains_ == 0 is finished.
    // Thus, to avoid inconsistencies between SimGrid and NS3, set remains to 0 only when the flow is finished in NS3
    double remains = action->get_cost() - sgFlow->sent_bytes_;
    if (remains > 0)
      action->set_remains(remains);

    if (TRACE_is_enabled() && action->get_state() == kernel::resource::Action::State::STARTED) {
      double data_delta_sent = sgFlow->sent_bytes_ - action->last_sent_;

      std::vector<StandardLinkImpl*> route;
      action->get_src().route_to(&action->get_dst(), route, nullptr);
      for (auto const* link : route)
        instr::resource_set_utilization("LINK", "bandwidth_used", link->get_cname(), action->get_category(),
                                        data_delta_sent / delta, now - delta, delta);

      action->last_sent_ = sgFlow->sent_bytes_;
    }

    if ((sgFlow->finished_) && (remains <= 0)) { // finished_ should not become true before remains gets to 0, but it
                                                 // sometimes does. Let's play safe, here.
      socket_to_destroy.push_back(ns3_socket);
      XBT_DEBUG("Destroy socket %s of action %p", ns3_socket.c_str(), action);
      action->set_remains(0);
      action->finish(Action::State::FINISHED);
    } else {
      XBT_DEBUG("Socket %s sent %u bytes out of %u (%u remaining)", ns3_socket.c_str(), sgFlow->sent_bytes_,
                sgFlow->total_bytes_, sgFlow->remaining_);
    }
  }

  while (not socket_to_destroy.empty()) {
    std::string ns3_socket = socket_to_destroy.back();
    socket_to_destroy.pop_back();
    SgFlow* flow = flow_from_sock.at(ns3_socket);
    if (XBT_LOG_ISENABLED(res_ns3, xbt_log_priority_debug)) {
      XBT_DEBUG("Removing socket %s of action %p", ns3_socket.c_str(), flow->action_);
    }
    delete flow;
    flow_from_sock.erase(ns3_socket);
  }
}

/************
 * Resource *
 ************/

LinkNS3::LinkNS3(const std::string& name, double bandwidth, s4u::Link::SharingPolicy sharing_policy,
                 routing::NetZoneImpl* englobing_zone)
    : StandardLinkImpl(name, sharing_policy, englobing_zone)
{
  bandwidth_.peak = bandwidth;
}

LinkNS3::~LinkNS3() = default;

void LinkNS3::apply_event(profile::Event*, double)
{
  THROW_UNIMPLEMENTED;
}

void LinkNS3::set_bandwidth_profile(profile::Profile* profile)
{
  xbt_assert(profile == nullptr, "The ns-3 network model doesn't support bandwidth profiles");
}

void LinkNS3::set_latency_profile(profile::Profile* profile)
{
  xbt_assert(profile == nullptr, "The ns-3 network model doesn't support latency profiles");
}

void LinkNS3::set_latency(double latency)
{
  latency_.peak = latency;
}

/**********
 * Action *
 **********/

NetworkNS3Action::NetworkNS3Action(Model* model, double totalBytes, s4u::Host* src, s4u::Host* dst)
    : NetworkAction(model, *src, *dst, totalBytes, false)
{
  // ns-3 fails when src = dst, so avoid the problem by considering that communications are infinitely fast on the
  // loopback that does not exists
  if (src == dst) {
    if (static bool warned = false; not warned) {
      XBT_WARN("Sending from a host %s to itself is not supported by ns-3. Every such communication finishes "
               "immediately upon startup in the SimGrid+ns-3 system.",
               src->get_cname());
      warned = true;
    }
    finish(Action::State::FINISHED);
    return;
  }

  // If there is no other started actions, we need to move NS-3 forward to be sync with SimGrid
  if (model->get_started_action_set()->size() == 1) {
    while (double_positive(EngineImpl::get_clock() - ns3::Simulator::Now().GetSeconds(), sg_precision_timing)) {
      XBT_DEBUG("Synchronizing NS-3 (time %f) with SimGrid (time %f)", ns3::Simulator::Now().GetSeconds(),
                EngineImpl::get_clock());
      ns3_simulator(EngineImpl::get_clock() - ns3::Simulator::Now().GetSeconds());
    }
  }

  static uint16_t port_number = 1;

  ns3::Ptr<ns3::Node> src_node = get_ns3node_from_sghost(src);
  ns3::Ptr<ns3::Node> dst_node = get_ns3node_from_sghost(dst);

  const std::string& addr = dst->get_netpoint()->extension<NetPointNs3>()->ipv4_address_;
  xbt_assert(not addr.empty(), "Element %s is unknown to ns-3. Is it connected to any one-hop link?",
             dst->get_netpoint()->get_cname());

  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port_number));
  sink.Install(dst_node);

  ns3::Ptr<ns3::Socket> sock = ns3::Socket::CreateSocket(src_node, ns3::TcpSocketFactory::GetTypeId());

  auto sock_addr = transform_socket_ptr(sock);
  XBT_DEBUG("Create socket %s for a flow of %.0f Bytes from %s to %s with Interface %s", sock_addr.c_str(), totalBytes,
            src->get_cname(), dst->get_cname(), addr.c_str());

  flow_from_sock.try_emplace(sock_addr, new SgFlow(static_cast<uint32_t>(totalBytes), this));

  sock->Bind(ns3::InetSocketAddress(port_number));

  ns3::Simulator::ScheduleNow(&start_flow, sock, addr.c_str(), port_number);

  port_number = 1 + (port_number % UINT16_MAX);
  if (port_number == 1)
    XBT_WARN("Too many connections! Port number is saturated. Trying to use the oldest ports.");
}

void NetworkNS3Action::suspend()
{
  THROW_UNIMPLEMENTED;
}

void NetworkNS3Action::resume()
{
  THROW_UNIMPLEMENTED;
}

std::list<StandardLinkImpl*> NetworkNS3Action::get_links() const
{
  THROW_UNIMPLEMENTED;
}
void NetworkNS3Action::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}

} // namespace kernel::resource

ns3::Ptr<ns3::Node> get_ns3node_from_sghost(const simgrid::s4u::Host* host)
{
  auto* netext = host->get_netpoint()->extension<NetPointNs3>();
  xbt_assert(netext != nullptr, "Please only use this function on ns-3 nodes");
  return netext->ns3_node_;
}
} // namespace simgrid

void ns3_simulator(double maxSeconds) // maxSecond is a delay, not an absolute time
{
  ns3::EventId id;
  if (maxSeconds >= 0.0) // If there is a maximum amount of time to run
    id = ns3::Simulator::Schedule(ns3::Seconds(maxSeconds), &ns3::Simulator::Stop);

  XBT_DEBUG("Start simulator for at most %fs (current simgrid time: %f; current ns3 time: %f)", maxSeconds,
            simgrid::kernel::EngineImpl::get_clock(), ns3::Simulator::Now().GetSeconds());
  ns3::Simulator::Run();
  XBT_DEBUG("ns3 simulator stopped at %fs", ns3::Simulator::Now().GetSeconds());

  if (maxSeconds >= 0.0)
    id.Cancel();
}

void ns3_add_direct_route(const simgrid::kernel::routing::NetPoint* src, const simgrid::kernel::routing::NetPoint* dst,
                          double bw, double lat, simgrid::s4u::Link::SharingPolicy policy)
{
  ns3::Ipv4AddressHelper address;
  ns3::NetDeviceContainer netA;

  // create link ns3
  auto* host_src = src->extension<NetPointNs3>();
  auto* host_dst = dst->extension<NetPointNs3>();

  xbt_assert(host_src != nullptr, "Network element %s does not seem to be ns-3-ready", src->get_cname());
  xbt_assert(host_dst != nullptr, "Network element %s does not seem to be ns-3-ready", dst->get_cname());

  xbt_assert(policy != simgrid::s4u::Link::SharingPolicy::WIFI,
             "The wifi sharing policy is not supported for links. You want to use a wifi zone (see documentation).");

  ns3::PointToPointHelper pointToPoint;

  XBT_DEBUG("\tAdd PTP from %s to %s bw:'%f Bps' lat:'%fs'", src->get_cname(), dst->get_cname(), bw, lat);
  pointToPoint.SetDeviceAttribute(
      "DataRate",
      ns3::DataRateValue(ns3::DataRate(static_cast<uint64_t>(bw * 8)))); // ns-3 takes bps, but we provide Bps
  pointToPoint.SetChannelAttribute("Delay", ns3::TimeValue(ns3::Seconds(lat)));

  netA.Add(pointToPoint.Install(host_src->ns3_node_, host_dst->ns3_node_));

  std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
  address.SetBase(addr.c_str(), "255.255.0.0");
  XBT_DEBUG("\tInterface stack '%s'", addr.c_str());

  auto addresses = address.Assign(netA);

  host_src->ipv4_address_ = transformIpv4Address(addresses.GetAddress(0));
  host_dst->ipv4_address_ = transformIpv4Address(addresses.GetAddress(1));

  if (number_of_links == 255) {
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  } else {
    number_of_links++;
  }
}
