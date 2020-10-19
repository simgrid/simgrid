/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <unordered_set>

#include "xbt/config.hpp"
#include "xbt/str.h"
#include "xbt/string.hpp"
#include "xbt/utility.hpp"

#include <ns3/core-module.h>
#include <ns3/csma-helper.h>
#include <ns3/global-route-manager.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/application-container.h>
#include <ns3/event-id.h>

#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"

#include "network_ns3.hpp"
#include "ns3/ns3_simulator.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/WifiZone.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ns3, surf, "Logging specific to the SURF network ns-3 module");

/*****************
 * Crude globals *
 *****************/

extern std::map<std::string, SgFlow*> flow_from_sock;
extern std::map<std::string, ns3::ApplicationContainer> sink_from_sock;

static ns3::InternetStackHelper stack;

static int number_of_links = 1;
static int number_of_networks = 1;

/* wifi globals */
static ns3::WifiHelper wifi;
static ns3::YansWifiPhyHelper wifiPhy         = ns3::YansWifiPhyHelper::Default();
static ns3::YansWifiChannelHelper wifiChannel = ns3::YansWifiChannelHelper::Default();
static ns3::WifiMacHelper wifiMac;
static ns3::MobilityHelper mobility;

simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> NetPointNs3::EXTENSION_ID;

static std::string transformIpv4Address(ns3::Ipv4Address from)
{
  std::stringstream sstream;
  sstream << from;
  return sstream.str();
}

NetPointNs3::NetPointNs3() : ns3_node_(ns3::CreateObject<ns3::Node>(0))
{
  stack.Install(ns3_node_);
}

WifiZone::WifiZone(const std::string& name_, simgrid::s4u::Host* host_, ns3::Ptr<ns3::Node> ap_node_,
                   ns3::Ptr<ns3::YansWifiChannel> channel_, int mcs_, int nss_, int network_, int link_)
    : name(name_)
    , host(host_)
    , ap_node(ap_node_)
    , channel(channel_)
    , mcs(mcs_)
    , nss(nss_)
    , network(network_)
    , link(link_)
{
  n_sta_nodes       = 0;
  wifi_zones[name_] = this;
}

bool WifiZone::is_ap(ns3::Ptr<ns3::Node> node)
{
  for (std::pair<std::string, WifiZone*> zone : wifi_zones)
    if (zone.second->get_ap_node() == node)
      return true;
  return false;
}

WifiZone* WifiZone::by_name(const std::string& name)
{
  WifiZone* zone;
  try {
    zone = wifi_zones.at(name);
  } catch (const std::out_of_range& oor) {
    return nullptr;
  }
  return zone;
}

std::unordered_map<std::string, WifiZone*> WifiZone::wifi_zones;

static void initialize_ns3_wifi()
{
  wifi.SetStandard(ns3::WIFI_PHY_STANDARD_80211n_5GHZ);

  for (auto host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    const char* wifi_link = host->get_property("wifi_link");
    const char* wifi_mcs  = host->get_property("wifi_mcs");
    const char* wifi_nss  = host->get_property("wifi_nss");

    if (wifi_link)
      new WifiZone(wifi_link, host, host->get_netpoint()->extension<NetPointNs3>()->ns3_node_, wifiChannel.Create(),
                   wifi_mcs ? atoi(wifi_mcs) : 3, wifi_nss ? atoi(wifi_nss) : 1, 0, 0);
  }
}

/*************
 * Callbacks *
 *************/

static void zoneCreation_cb(simgrid::s4u::NetZone const& zone) {
    simgrid::kernel::routing::WifiZone* wifizone = dynamic_cast<simgrid::kernel::routing::WifiZone*> (zone.get_impl());
    if (wifizone == nullptr) return;

    wifi.SetStandard(ns3::WIFI_PHY_STANDARD_80211n_5GHZ);

    std::string ssid = wifizone->get_name();
    const char* mcs = wifizone->get_property("mcs");
    const char* nss = wifizone->get_property("nss");
    int mcs_value = mcs ? atoi(mcs) : 3;
    int nss_value = nss ? atoi(nss) : 1;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "ControlMode", ns3::StringValue("HtMcs0"),
                                 "DataMode", ns3::StringValue("HtMcs" + std::to_string(mcs_value)));
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("Antennas", ns3::UintegerValue(nss_value));
    wifiPhy.Set("MaxSupportedTxSpatialStreams", ns3::UintegerValue(nss_value));
    wifiPhy.Set("MaxSupportedRxSpatialStreams", ns3::UintegerValue(nss_value));
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", ns3::SsidValue(ssid));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ns3::Ptr<ns3::ListPositionAllocator> positionAllocS = ns3::CreateObject<ns3::ListPositionAllocator>();
    positionAllocS->Add(ns3::Vector(0, 0, 0));

    ns3::NetDeviceContainer netDevices;
    NetPointNs3* access_point_netpoint_ns3 = wifizone->get_access_point()->extension<NetPointNs3>();

    ns3::Ptr<ns3::Node> access_point_ns3_node = access_point_netpoint_ns3->ns3_node_;
    ns3::NodeContainer nodes = {access_point_ns3_node};
    std::vector<NetPointNs3*> hosts_netpoints = {access_point_netpoint_ns3};
    netDevices.Add(wifi.Install(wifiPhy, wifiMac,access_point_ns3_node));

    wifiMac.SetType ("ns3::StaWifiMac",
                     "Ssid", ns3::SsidValue(ssid),
                     "ActiveProbing", ns3::BooleanValue(false));

    NetPointNs3* station_netpoint_ns3 = nullptr;
    ns3::Ptr<ns3::Node> station_ns3_node = nullptr;
    const char* distance;
    for (auto station_host : wifizone->get_all_hosts()) {
        station_netpoint_ns3 = station_host->get_netpoint()->extension<NetPointNs3>();
        if (station_netpoint_ns3 == access_point_netpoint_ns3)
            continue;
        hosts_netpoints.push_back(station_netpoint_ns3);
        distance = station_host->get_property("wifi_distance");
        positionAllocS->Add(ns3::Vector(distance ? atof(distance) : 10.0, 0, 0));
        station_ns3_node = station_netpoint_ns3->ns3_node_;
        nodes.Add(station_ns3_node);
        netDevices.Add(wifi.Install(wifiPhy, wifiMac, station_ns3_node));
    }

    ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(40));

    mobility.SetPositionAllocator(positionAllocS);
    mobility.Install(nodes);

    ns3::Ipv4AddressHelper address;
    std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
    address.SetBase(addr.c_str(), "255.255.0.0");
    XBT_DEBUG("\tInterface stack '%s'", addr.c_str());
    ns3::Ipv4InterfaceContainer addresses = address.Assign(netDevices);
    for (int i = 0; i < hosts_netpoints.size(); i++) {
        hosts_netpoints[i]->ipv4_address_ = transformIpv4Address(addresses.GetAddress(i));
    }

    if (number_of_links == 255) {
      xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
      number_of_links = 1;
      number_of_networks++;
    } else {
      number_of_links++;
    }
}

static void clusterCreation_cb(simgrid::kernel::routing::ClusterCreationArgs const& cluster)
{
  ns3::NodeContainer Nodes;

  for (int const& i : *cluster.radicals) {
    // Create private link
    std::string host_id = cluster.prefix + std::to_string(i) + cluster.suffix;
    auto* src           = simgrid::s4u::Host::by_name(host_id)->get_netpoint();
    auto* dst           = simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(cluster.router_id);
    xbt_assert(dst != nullptr, "No router named %s", cluster.router_id.c_str());

    ns3_add_direct_route(src, dst, cluster.bw, cluster.lat, cluster.id,
                         cluster.sharing_policy); // Any ns-3 route is symmetrical

    // Also add the host to the list of hosts that will be connected to the backbone
    Nodes.Add(src->extension<NetPointNs3>()->ns3_node_);
  }

  // Create link backbone

  xbt_assert(Nodes.GetN() <= 65000, "Cluster with ns-3 is limited to 65000 nodes");
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute("DataRate",
                           ns3::DataRateValue(ns3::DataRate(cluster.bb_bw * 8))); // ns-3 takes bps, but we provide Bps
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

static void routeCreation_cb(bool symmetrical, simgrid::kernel::routing::NetPoint* src,
                             simgrid::kernel::routing::NetPoint* dst, simgrid::kernel::routing::NetPoint* /*gw_src*/,
                             simgrid::kernel::routing::NetPoint* /*gw_dst*/,
                             std::vector<simgrid::kernel::resource::LinkImpl*> const& link_list)
{
  if (link_list.size() == 1) {
    auto const* link = static_cast<simgrid::kernel::resource::LinkNS3*>(link_list[0]);

    XBT_DEBUG("Route from '%s' to '%s' with link '%s' %s %s", src->get_cname(), dst->get_cname(), link->get_cname(),
              (link->get_sharing_policy() == simgrid::s4u::Link::SharingPolicy::WIFI ? "(wifi)" : "(wired)"),
              (symmetrical ? "(symmetrical)" : "(not symmetrical)"));

    //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
    XBT_DEBUG("\tLink (%s) bw:%fbps lat:%fs", link->get_cname(), link->get_bandwidth(), link->get_latency());

    ns3_add_direct_route(src, dst, link->get_bandwidth(), link->get_latency(), link->get_name(),
                         link->get_sharing_policy());
  } else {
    static bool warned_about_long_routes = false;

    if (not warned_about_long_routes)
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
void surf_network_model_init_NS3()
{
  xbt_assert(surf_network_model == nullptr, "Cannot set the network model twice");

  surf_network_model = new simgrid::kernel::resource::NetworkNS3Model();
}

static simgrid::config::Flag<std::string>
    ns3_tcp_model("ns3/TcpModel", "The ns-3 tcp model can be: NewReno or Reno or Tahoe", "default");
static simgrid::config::Flag<std::string> ns3_seed(
    "ns3/seed",
    "The random seed provided to ns-3. Either 'time' to seed with time(), blank to not set (default), or a number.", "",
    [](std::string val) {
      if (val.length() == 0)
        return;
      if (strcasecmp(val.c_str(), "time") == 0) {
        std::srand(time(NULL));
        ns3::RngSeedManager::SetSeed(std::rand());
        ns3::RngSeedManager::SetRun(std::rand());
      } else {
        int v = xbt_str_parse_int(
            val.c_str(), "Invalid value for option ns3/seed. It must be either 'time', a number, or left empty.");
        ns3::RngSeedManager::SetSeed(v);
        ns3::RngSeedManager::SetRun(v);
      }
    });

namespace simgrid {
namespace kernel {
namespace resource {

NetworkNS3Model::NetworkNS3Model() : NetworkModel(Model::UpdateAlgo::FULL)
{
  xbt_assert(not sg_link_energy_is_inited(),
             "LinkEnergy plugin and ns-3 network models are not compatible. Are you looking for Ecofen, maybe?");

  all_existing_models.push_back(this);

  NetPointNs3::EXTENSION_ID = routing::NetPoint::extension_create<NetPointNs3>();

  ns3::Config::SetDefault("ns3::TcpSocket::SegmentSize", ns3::UintegerValue(1000));
  ns3::Config::SetDefault("ns3::TcpSocket::DelAckCount", ns3::UintegerValue(1));
  ns3::Config::SetDefault("ns3::TcpSocketBase::Timestamp", ns3::BooleanValue(false));

  auto TcpProtocol = ns3_tcp_model.get();
  if (TcpProtocol == "default") {
    /* nothing to do */

  } else if (TcpProtocol == "Reno" || TcpProtocol == "NewReno" || TcpProtocol == "Tahoe") {
    XBT_INFO("Switching Tcp protocol to '%s'", TcpProtocol.c_str());
    ns3::Config::SetDefault("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::Tcp" + TcpProtocol));

  } else {
    xbt_die("The ns3/TcpModel must be: NewReno or Reno or Tahoe");
  }

  routing::NetPoint::on_creation.connect([](routing::NetPoint& pt) {
    pt.extension_set<NetPointNs3>(new NetPointNs3());
    XBT_VERB("Declare SimGrid's %s within ns-3", pt.get_cname());
  });

  s4u::Engine::on_platform_created.connect([]() {
    /* Create the ns3 topology based on routing strategy */
    ns3::GlobalRouteManager::BuildGlobalRoutingDatabase();
    ns3::GlobalRouteManager::InitializeRoutes();
  });
  routing::on_cluster_creation.connect(&clusterCreation_cb);
  s4u::NetZone::on_route_creation.connect(&routeCreation_cb);
  s4u::NetZone::on_seal.connect(&zoneCreation_cb);
}

LinkImpl* NetworkNS3Model::create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                                       s4u::Link::SharingPolicy policy)
{
  xbt_assert(bandwidths.size() == 1, "ns-3 links must use only 1 bandwidth.");
  return new LinkNS3(this, name, bandwidths[0], latency, policy);
}

Action* NetworkNS3Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  xbt_assert(rate == -1,
             "Communication over ns-3 links cannot specify a specific rate. Please use -1 as a value instead of %f.",
             rate);
  return new NetworkNS3Action(this, size, src, dst);
}

double NetworkNS3Model::next_occurring_event(double now)
{
  double time_to_next_flow_completion = 0.0;
  XBT_DEBUG("ns3_next_occurring_event");

  //get the first relevant value from the running_actions list

  // If there is no comms in NS-3, then we do not move it forward.
  // We will synchronize NS-3 with SimGrid when starting a new communication.
  // (see NetworkNS3Action::NetworkNS3Action() for more details on this point)
  if (get_started_action_set()->empty() || now == 0.0)
    return -1.0;

  XBT_DEBUG("doing a ns3 simulation for a duration of %f", now);
  ns3_simulator(now);
  time_to_next_flow_completion = ns3::Simulator::Now().GetSeconds() - surf_get_clock();
  // NS-3 stops as soon as a flow ends,
  // but it does not process the other flows that may finish at the same (simulated) time.
  // If another flow ends at the same time, time_to_next_flow_completion = 0
  if(double_equals(time_to_next_flow_completion, 0, sg_surf_precision))
    time_to_next_flow_completion = 0.0;

  XBT_DEBUG("min       : %f", now);
  XBT_DEBUG("ns3  time : %f", ns3::Simulator::Now().GetSeconds());
  XBT_DEBUG("surf time : %f", surf_get_clock());
  XBT_DEBUG("Next completion %f :", time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

void NetworkNS3Model::update_actions_state(double now, double delta)
{
  static std::vector<std::string> socket_to_destroy;

  std::string ns3_socket;
  for (const auto& elm : flow_from_sock) {
    ns3_socket                = elm.first;
    SgFlow* sgFlow            = elm.second;
    NetworkNS3Action * action = sgFlow->action_;
    XBT_DEBUG("Processing socket %p (action %p)",sgFlow,action);
    // Because NS3 stops as soon as a flow is finished, the other flows that ends at the same time may remains in an
    // inconsistent state (i.e. remains_ == 0 but finished_ == false).
    // However, SimGrid considers sometimes that an action with remains_ == 0 is finished.
    // Thus, to avoid inconsistencies between SimGrid and NS3, set remains to 0 only when the flow is finished in NS3
    int remains = action->get_cost() - sgFlow->sent_bytes_;
    if(remains > 0)
      action->set_remains(remains);

    if (TRACE_is_enabled() && action->get_state() == kernel::resource::Action::State::STARTED) {
      double data_delta_sent = sgFlow->sent_bytes_ - action->last_sent_;

      std::vector<LinkImpl*> route = std::vector<LinkImpl*>();

      action->get_src().route_to(&action->get_dst(), route, nullptr);
      for (auto const& link : route)
        instr::resource_set_utilization("LINK", "bandwidth_used", link->get_cname(), action->get_category(),
                                        (data_delta_sent) / delta, now - delta, delta);

      action->last_sent_ = sgFlow->sent_bytes_;
    }

    if ((sgFlow->finished_) && (remains <= 0)) { // finished_ should not become true before remains gets to 0, but it sometimes does. Let's play safe, here.
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
    ns3_socket = socket_to_destroy.back();
    socket_to_destroy.pop_back();
    SgFlow* flow = flow_from_sock.at(ns3_socket);
    if (XBT_LOG_ISENABLED(ns3, xbt_log_priority_debug)) {
      XBT_DEBUG("Removing socket %s of action %p", ns3_socket.c_str(), flow->action_);
    }
    delete flow;
    flow_from_sock.erase(ns3_socket);
    sink_from_sock.erase(ns3_socket);
  }
}

/************
 * Resource *
 ************/

LinkNS3::LinkNS3(NetworkNS3Model* model, const std::string& name, double bandwidth, double latency,
                 s4u::Link::SharingPolicy policy)
    : LinkImpl(model, name, nullptr), sharing_policy_(policy)
{
  bandwidth_.peak = bandwidth;
  latency_.peak   = latency;

  /* If wifi, create the wifizone now. If not, don't do anything: the links will be created in routeCreate_cb */

  if (policy == simgrid::s4u::Link::SharingPolicy::WIFI) {
    static bool wifi_init = false;
    if (!wifi_init) {
      initialize_ns3_wifi();
      wifi_init = true;
    }

    ns3::NetDeviceContainer netA;
    WifiZone* zone = WifiZone::by_name(name);
    xbt_assert(zone != nullptr, "Link name '%s' does not match the 'wifi_link' property of a host.", name.c_str());
    auto* netpoint_ns3 = zone->get_host()->get_netpoint()->extension<NetPointNs3>();

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "ControlMode", ns3::StringValue("HtMcs0"), "DataMode",
                                 ns3::StringValue("HtMcs" + std::to_string(zone->get_mcs())));

    wifiPhy.SetChannel(zone->get_channel());
    wifiPhy.Set("Antennas", ns3::UintegerValue(zone->get_nss()));
    wifiPhy.Set("MaxSupportedTxSpatialStreams", ns3::UintegerValue(zone->get_nss()));
    wifiPhy.Set("MaxSupportedRxSpatialStreams", ns3::UintegerValue(zone->get_nss()));

    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", ns3::SsidValue(name));

    netA.Add(wifi.Install(wifiPhy, wifiMac, zone->get_ap_node()));

    ns3::Ptr<ns3::ListPositionAllocator> positionAllocS = ns3::CreateObject<ns3::ListPositionAllocator>();
    positionAllocS->Add(ns3::Vector(0, 0, 0));
    mobility.SetPositionAllocator(positionAllocS);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(zone->get_ap_node());

    ns3::Ipv4AddressHelper address;
    std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
    address.SetBase(addr.c_str(), "255.255.0.0");
    XBT_DEBUG("\tInterface stack '%s'", addr.c_str());
    auto addresses = address.Assign(netA);
    zone->set_network(number_of_networks);
    zone->set_link(number_of_links);

    netpoint_ns3->ipv4_address_ = transformIpv4Address(addresses.GetAddress(1));

    if (number_of_links == 255) {
      xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
      number_of_links = 1;
      number_of_networks++;
    } else {
      number_of_links++;
    }
  }
  s4u::Link::on_creation(*this->get_iface());
}

LinkNS3::~LinkNS3() = default;

void LinkNS3::apply_event(profile::Event*, double)
{
  THROW_UNIMPLEMENTED;
}
void LinkNS3::set_bandwidth_profile(profile::Profile*)
{
  xbt_die("The ns-3 network model doesn't support bandwidth profiles");
}
void LinkNS3::set_latency_profile(profile::Profile*)
{
  xbt_die("The ns-3 network model doesn't support latency profiles");
}

/**********
 * Action *
 **********/

NetworkNS3Action::NetworkNS3Action(Model* model, double totalBytes, s4u::Host* src, s4u::Host* dst)
    : NetworkAction(model, *src, *dst, totalBytes, false)
{
  // If there is no other started actions, we need to move NS-3 forward to be sync with SimGrid
  if (model->get_started_action_set()->size()==1){
    while(double_positive(surf_get_clock() - ns3::Simulator::Now().GetSeconds(), sg_surf_precision)){
      XBT_DEBUG("Synchronizing NS-3 (time %f) with SimGrid (time %f)", ns3::Simulator::Now().GetSeconds(), surf_get_clock());
      ns3_simulator(surf_get_clock() - ns3::Simulator::Now().GetSeconds());
    }
  }

  static int port_number = 1025; // Port number is limited from 1025 to 65 000

  ns3::Ptr<ns3::Node> src_node = src->get_netpoint()->extension<NetPointNs3>()->ns3_node_;
  ns3::Ptr<ns3::Node> dst_node = dst->get_netpoint()->extension<NetPointNs3>()->ns3_node_;

  std::string& addr = dst->get_netpoint()->extension<NetPointNs3>()->ipv4_address_;
  xbt_assert(not addr.empty(), "Element %s is unknown to ns-3. Is it connected to any one-hop link?",
             dst->get_netpoint()->get_cname());

  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port_number));
  ns3::ApplicationContainer apps = sink.Install(dst_node);

  ns3::Ptr<ns3::Socket> sock = ns3::Socket::CreateSocket(src_node, ns3::TcpSocketFactory::GetTypeId());

  XBT_DEBUG("Create socket %s for a flow of %.0f Bytes from %s to %s with Interface %s",
            transform_socket_ptr(sock).c_str(), totalBytes, src->get_cname(), dst->get_cname(), addr.c_str());

  flow_from_sock.insert({transform_socket_ptr(sock), new SgFlow(totalBytes, this)});
  sink_from_sock.insert({transform_socket_ptr(sock), apps});

  sock->Bind(ns3::InetSocketAddress(port_number));

  ns3::Simulator::ScheduleNow(&start_flow, sock, addr.c_str(), port_number);

  port_number++;
  if(port_number > 65000){
    port_number = 1025;
    XBT_WARN("Too many connections! Port number is saturated. Trying to use the oldest ports.");
  }

  s4u::Link::on_communicate(*this);
}

void NetworkNS3Action::suspend() {
  THROW_UNIMPLEMENTED;
}

void NetworkNS3Action::resume() {
  THROW_UNIMPLEMENTED;
}

std::list<LinkImpl*> NetworkNS3Action::get_links() const
{
  THROW_UNIMPLEMENTED;
}
void NetworkNS3Action::update_remains_lazy(double /*now*/)
{
  THROW_IMPOSSIBLE;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid

void ns3_simulator(double maxSeconds)
{
  ns3::EventId id;
  if (maxSeconds > 0.0) // If there is a maximum amount of time to run
    id = ns3::Simulator::Schedule(ns3::Seconds(maxSeconds), &ns3::Simulator::Stop);

  XBT_DEBUG("Start simulator for at most %fs (current time: %f)", maxSeconds, surf_get_clock());
  ns3::Simulator::Run ();
  XBT_DEBUG("Simulator stopped at %fs", ns3::Simulator::Now().GetSeconds());

  if(maxSeconds > 0.0)
    id.Cancel();
}

void ns3_add_direct_route(simgrid::kernel::routing::NetPoint* src, simgrid::kernel::routing::NetPoint* dst, double bw,
                          double lat, const std::string& link_name, simgrid::s4u::Link::SharingPolicy policy)
{
  ns3::Ipv4AddressHelper address;
  ns3::NetDeviceContainer netA;

  // create link ns3
  auto* host_src = src->extension<NetPointNs3>();
  auto* host_dst = dst->extension<NetPointNs3>();

  xbt_assert(host_src != nullptr, "Network element %s does not seem to be ns-3-ready", src->get_cname());
  xbt_assert(host_dst != nullptr, "Network element %s does not seem to be ns-3-ready", dst->get_cname());

  if (policy == simgrid::s4u::Link::SharingPolicy::WIFI) {
    auto a = host_src->ns3_node_;
    auto b = host_dst->ns3_node_;
    xbt_assert(WifiZone::is_ap(a) != WifiZone::is_ap(b),
               "A wifi route can only exist between an access point node and a station node.");

    ns3::Ptr<ns3::Node> apNode  = WifiZone::is_ap(a) ? a : b;
    ns3::Ptr<ns3::Node> staNode = apNode == a ? b : a;

    WifiZone* zone = WifiZone::by_name(link_name);

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "ControlMode", ns3::StringValue("HtMcs0"), "DataMode",
                                 ns3::StringValue("HtMcs" + std::to_string(zone->get_mcs())));

    wifiPhy.SetChannel(zone->get_channel());
    wifiPhy.Set("Antennas", ns3::UintegerValue(zone->get_nss()));
    wifiPhy.Set("MaxSupportedTxSpatialStreams", ns3::UintegerValue(zone->get_nss()));
    wifiPhy.Set("MaxSupportedRxSpatialStreams", ns3::UintegerValue(zone->get_nss()));

    wifiMac.SetType ("ns3::StaWifiMac",
                     "Ssid", ns3::SsidValue(link_name),
                     "ActiveProbing", ns3::BooleanValue(false));

    netA.Add(wifi.Install(wifiPhy, wifiMac, staNode));

    ns3::Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", ns3::UintegerValue(40));

    simgrid::kernel::routing::NetPoint* sta_netpoint = WifiZone::is_ap(host_src->ns3_node_) ? dst : src;
    const char* wifi_distance    = simgrid::s4u::Host::by_name(sta_netpoint->get_name())->get_property("wifi_distance");
    ns3::Ptr<ns3::ListPositionAllocator> positionAllocS = ns3::CreateObject<ns3::ListPositionAllocator>();
    positionAllocS->Add(ns3::Vector(wifi_distance ? atof(wifi_distance) : 10.0, 0, 0));
    mobility.SetPositionAllocator(positionAllocS);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(staNode);

    std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", zone->get_network(), zone->get_link());
    address.SetBase(addr.c_str(), "255.255.0.0", ("0.0.0." + std::to_string(zone->get_n_sta_nodes() + 2)).c_str());
    zone->add_sta_node();
    XBT_DEBUG("\tInterface stack '%s'", addr.c_str());
    auto addresses          = address.Assign(netA);
    host_dst->ipv4_address_ = transformIpv4Address(addresses.GetAddress(1));
  } else {
    ns3::PointToPointHelper pointToPoint;

    XBT_DEBUG("\tAdd PTP from %s to %s bw:'%f Bps' lat:'%fs'", src->get_cname(), dst->get_cname(), bw, lat);
    pointToPoint.SetDeviceAttribute("DataRate",
                                    ns3::DataRateValue(ns3::DataRate(bw * 8))); // ns-3 takes bps, but we provide Bps
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
}
