/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <unordered_set>

#include "xbt/config.hpp"
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

#include "network_ns3.hpp"
#include "ns3/ns3_simulator.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ns3, surf, "Logging specific to the SURF network ns-3 module");

std::vector<std::string> IPV4addr;

/*****************
 * Crude globals *
 *****************/

extern std::map<std::string, SgFlow*> flow_from_sock;
extern std::map<std::string, ns3::ApplicationContainer> sink_from_sock;

static ns3::InternetStackHelper stack;
static ns3::NodeContainer nodes;
static ns3::NodeContainer Cluster_nodes;
static ns3::Ipv4InterfaceContainer interfaces;

static int number_of_nodes = 0;
static int number_of_clusters_nodes = 0;
static int number_of_links = 1;
static int number_of_networks = 1;

simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> NetPointNs3::EXTENSION_ID;

NetPointNs3::NetPointNs3() : ns3_node_(ns3::CreateObject<ns3::Node>(0))
{
  stack.Install(ns3_node_);
  nodes.Add(ns3_node_);
  node_num = number_of_nodes++;
}

/*************
 * Callbacks *
 *************/

static void clusterCreation_cb(simgrid::kernel::routing::ClusterCreationArgs const& cluster)
{
  for (int const& i : *cluster.radicals) {
    // Routers don't create a router on the other end of the private link by themselves.
    // We just need this router to be given an ID so we create a temporary NetPointNS3 so that it gets one
    auto* host_dst = new NetPointNs3();

    // Create private link
    std::string host_id = cluster.prefix + std::to_string(i) + cluster.suffix;
    auto* host_src      = simgrid::s4u::Host::by_name(host_id)->get_netpoint()->extension<NetPointNs3>();
    xbt_assert(host_src, "Cannot find a ns-3 host of name %s", host_id.c_str());

    // Any ns-3 route is symmetrical
    ns3_add_link(host_src, host_dst, cluster.bw, cluster.lat);

    delete host_dst;
  }

  //Create link backbone
  ns3_add_cluster(cluster.id.c_str(), cluster.bb_bw, cluster.bb_lat);
}

static void routeCreation_cb(bool symmetrical, simgrid::kernel::routing::NetPoint* src,
                             simgrid::kernel::routing::NetPoint* dst, simgrid::kernel::routing::NetPoint* gw_src,
                             simgrid::kernel::routing::NetPoint* gw_dst,
                             std::vector<simgrid::kernel::resource::LinkImpl*> const& link_list)
{
  if (link_list.size() == 1) {
    auto* link = static_cast<simgrid::kernel::resource::LinkNS3*>(link_list[0]);

    XBT_DEBUG("Route from '%s' to '%s' with link '%s' %s", src->get_cname(), dst->get_cname(), link->get_cname(),
              (symmetrical ? "(symmetrical)" : "(not symmetrical)"));

    //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
    XBT_DEBUG("\tLink (%s) bw:%fbps lat:%fs", link->get_cname(), link->get_bandwidth(), link->get_latency());

    // create link ns3
    auto* host_src = src->extension<NetPointNs3>();
    auto* host_dst = dst->extension<NetPointNs3>();

    xbt_assert(host_src != nullptr, "Network element %s does not seem to be ns-3-ready", src->get_cname());
    xbt_assert(host_dst != nullptr, "Network element %s does not seem to be ns-3-ready", dst->get_cname());

    ns3_add_link(host_src, host_dst, link->get_bandwidth(), link->get_latency());
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

/* Create the ns3 topology based on routing strategy */
static void postparse_cb()
{
  IPV4addr.shrink_to_fit();

  ns3::GlobalRouteManager::BuildGlobalRoutingDatabase();
  ns3::GlobalRouteManager::InitializeRoutes();
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
    ns3_tcp_model("ns3/TcpModel", "The ns-3 tcp model can be : NewReno or Reno or Tahoe", "default");

namespace simgrid {
namespace kernel {
namespace resource {

NetworkNS3Model::NetworkNS3Model() : NetworkModel(Model::UpdateAlgo::FULL)
{
  xbt_assert(not sg_link_energy_is_inited(),
             "LinkEnergy plugin and ns-3 network models are not compatible. Are you looking for Ecofen, maybe?");

  all_existing_models.push_back(this);

  NetPointNs3::EXTENSION_ID = routing::NetPoint::extension_create<NetPointNs3>();

  ns3_initialize(ns3_tcp_model.get());

  routing::NetPoint::on_creation.connect([](routing::NetPoint& pt) {
    pt.extension_set<NetPointNs3>(new NetPointNs3());
    XBT_VERB("SimGrid's %s is known as node %d within ns-3", pt.get_cname(), pt.extension<NetPointNs3>()->node_num);
  });
  routing::on_cluster_creation.connect(&clusterCreation_cb);

  s4u::Engine::on_platform_created.connect(&postparse_cb);
  s4u::NetZone::on_route_creation.connect(&routeCreation_cb);
}

NetworkNS3Model::~NetworkNS3Model() {
  IPV4addr.clear();
}

LinkImpl* NetworkNS3Model::create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                                       s4u::Link::SharingPolicy)
{
  xbt_assert(bandwidths.size() == 1, "ns-3 links must use only 1 bandwidth.");
  return new LinkNS3(this, name, bandwidths[0], latency);
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

      action->src_->route_to(action->dst_, route, nullptr);
      for (auto const& link : route)
        instr::resource_set_utilization("LINK", "bandwidth_used", link->get_cname(), action->get_category(),
                                        (data_delta_sent) / delta, now - delta, delta);

      action->last_sent_ = sgFlow->sent_bytes_;
    }

    if(sgFlow->finished_){
      socket_to_destroy.push_back(ns3_socket);
      XBT_DEBUG("Destroy socket %p of action %p", ns3_socket.c_str(), action);
      action->set_remains(0);
      action->finish(Action::State::FINISHED);
    } else {
      XBT_DEBUG("Socket %p sent %u bytes out of %u (%u remaining)", ns3_socket.c_str(), sgFlow->sent_bytes_,
                sgFlow->total_bytes_, sgFlow->remaining_);
    }
  }

  while (not socket_to_destroy.empty()) {
    ns3_socket = socket_to_destroy.back();
    socket_to_destroy.pop_back();
    SgFlow* flow = flow_from_sock.at(ns3_socket);
    if (XBT_LOG_ISENABLED(ns3, xbt_log_priority_debug)) {
      XBT_DEBUG("Removing socket %p of action %p", ns3_socket.c_str(), flow->action_);
    }
    delete flow;
    flow_from_sock.erase(ns3_socket);
    sink_from_sock.erase(ns3_socket);
  }
}

/************
 * Resource *
 ************/

LinkNS3::LinkNS3(NetworkNS3Model* model, const std::string& name, double bandwidth, double latency)
    : LinkImpl(model, name, nullptr)
{
  bandwidth_.peak = bandwidth;
  latency_.peak   = latency;

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
    : NetworkAction(model, totalBytes, false), src_(src), dst_(dst)
{
  // If there is no other started actions, we need to move NS-3 forward to be sync with SimGrid
  if (model->get_started_action_set()->size()==1){
    while(double_positive(surf_get_clock() - ns3::Simulator::Now().GetSeconds(), sg_surf_precision)){
      XBT_DEBUG("Synchronizing NS-3 (time %f) with SimGrid (time %f)", ns3::Simulator::Now().GetSeconds(), surf_get_clock());
      ns3_simulator(surf_get_clock() - ns3::Simulator::Now().GetSeconds());
    }
  }

  XBT_DEBUG("Communicate from %s to %s", src->get_cname(), dst->get_cname());

  static int port_number = 1025; // Port number is limited from 1025 to 65 000

  unsigned int node1 = src->get_netpoint()->extension<NetPointNs3>()->node_num;
  unsigned int node2 = dst->get_netpoint()->extension<NetPointNs3>()->node_num;

  ns3::Ptr<ns3::Node> src_node = src->get_netpoint()->extension<NetPointNs3>()->ns3_node_;
  ns3::Ptr<ns3::Node> dst_node = dst->get_netpoint()->extension<NetPointNs3>()->ns3_node_;

  xbt_assert(node2 < IPV4addr.size(), "Element %s is unknown to ns-3. Is it connected to any one-hop link?",
             dst->get_netpoint()->get_cname());
  std::string& addr = IPV4addr[node2];
  xbt_assert(not addr.empty(), "Element %s is unknown to ns-3. Is it connected to any one-hop link?",
             dst->get_netpoint()->get_cname());

  XBT_DEBUG("ns3: Create flow of %.0f Bytes from %u to %u with Interface %s", totalBytes, node1, node2, addr.c_str());
  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port_number));
  ns3::ApplicationContainer apps = sink.Install(dst_node);

  ns3::Ptr<ns3::Socket> sock = ns3::Socket::CreateSocket(src_node, ns3::TcpSocketFactory::GetTypeId());

  flow_from_sock.insert({transform_socket_ptr(sock), new SgFlow(totalBytes, this)});
  sink_from_sock.insert({transform_socket_ptr(sock), apps});

  sock->Bind(ns3::InetSocketAddress(port_number));

  ns3::Simulator::ScheduleNow(&start_flow, sock, addr.c_str(), port_number);

  port_number++;
  if(port_number > 65000){
    port_number = 1025;
    XBT_WARN("Too many connections! Port number is saturated. Trying to use the oldest ports.");
  }
  xbt_assert(port_number <= 65000, "Too many connections! Port number is saturated.");

  s4u::Link::on_communicate(*this, src, dst);
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

// initialize the ns-3 interface and environment
void ns3_initialize(std::string TcpProtocol)
{
  //  tcpModel are:
  //  "ns3::TcpNewReno"
  //  "ns3::TcpReno"
  //  "ns3::TcpTahoe"

  ns3::Config::SetDefault ("ns3::TcpSocket::SegmentSize", ns3::UintegerValue (1000));
  ns3::Config::SetDefault ("ns3::TcpSocket::DelAckCount", ns3::UintegerValue (1));
  ns3::Config::SetDefault ("ns3::TcpSocketBase::Timestamp", ns3::BooleanValue (false));

  if (TcpProtocol == "default") {
    /* nothing to do */

  } else if (TcpProtocol == "Reno") {
    XBT_INFO("Switching Tcp protocol to '%s'", TcpProtocol.c_str());
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpReno"));

  } else if (TcpProtocol == "NewReno") {
    XBT_INFO("Switching Tcp protocol to '%s'", TcpProtocol.c_str());
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpNewReno"));

  } else if (TcpProtocol == "Tahoe") {
    XBT_INFO("Switching Tcp protocol to '%s'", TcpProtocol.c_str());
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpTahoe"));

  } else {
    xbt_die("The ns3/TcpModel must be: NewReno or Reno or Tahoe");
  }
}

void ns3_add_cluster(const char* id, double bw, double lat) {
  ns3::NodeContainer Nodes;

  for (unsigned int i = number_of_clusters_nodes; i < Cluster_nodes.GetN(); i++) {
    Nodes.Add(Cluster_nodes.Get(i));
    XBT_DEBUG("Add node %u to cluster", i);
  }
  number_of_clusters_nodes = Cluster_nodes.GetN();

  XBT_DEBUG("Add router %u to cluster", nodes.GetN() - Nodes.GetN() - 1);
  Nodes.Add(nodes.Get(nodes.GetN()-Nodes.GetN()-1));

  xbt_assert(Nodes.GetN() <= 65000, "Cluster with ns-3 is limited to 65000 nodes");
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", ns3::DataRateValue(ns3::DataRate(bw * 8))); // ns-3 takes bps, but we provide Bps
  csma.SetChannelAttribute("Delay", ns3::TimeValue(ns3::Seconds(lat)));
  ns3::NetDeviceContainer devices = csma.Install(Nodes);
  XBT_DEBUG("Create CSMA");

  std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
  XBT_DEBUG("Assign IP Addresses %s to CSMA.", addr.c_str());
  ns3::Ipv4AddressHelper ipv4;
  ipv4.SetBase(addr.c_str(), "255.255.0.0");
  interfaces.Add(ipv4.Assign (devices));

  if(number_of_links == 255){
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  }else{
    number_of_links++;
  }
  XBT_DEBUG("Number of nodes in Cluster_nodes: %u", Cluster_nodes.GetN());
}

static std::string transformIpv4Address(ns3::Ipv4Address from)
{
  std::stringstream sstream;
  sstream << from ;
  return sstream.str();
}

void ns3_add_link(NetPointNs3* src, NetPointNs3* dst, double bw, double lat) {
  ns3::PointToPointHelper pointToPoint;

  ns3::Ipv4AddressHelper address;

  int srcNum = src->node_num;
  int dstNum = dst->node_num;

  ns3::Ptr<ns3::Node> a = src->ns3_node_;
  ns3::Ptr<ns3::Node> b = dst->ns3_node_;

  XBT_DEBUG("\tAdd PTP from %d to %d bw:'%f Bps' lat:'%fs'", srcNum, dstNum, bw, lat);
  pointToPoint.SetDeviceAttribute("DataRate",
                                  ns3::DataRateValue(ns3::DataRate(bw * 8))); // ns-3 takes bps, but we provide Bps
  pointToPoint.SetChannelAttribute("Delay", ns3::TimeValue(ns3::Seconds(lat)));

  ns3::NetDeviceContainer netA;
  netA.Add(pointToPoint.Install (a, b));

  std::string addr = simgrid::xbt::string_printf("%d.%d.0.0", number_of_networks, number_of_links);
  address.SetBase(addr.c_str(), "255.255.0.0");
  XBT_DEBUG("\tInterface stack '%s'", addr.c_str());
  interfaces.Add(address.Assign (netA));

  if (IPV4addr.size() <= (unsigned)srcNum)
    IPV4addr.resize(srcNum + 1);
  IPV4addr[srcNum] = transformIpv4Address(interfaces.GetAddress(interfaces.GetN() - 2));

  if (IPV4addr.size() <= (unsigned)dstNum)
    IPV4addr.resize(dstNum + 1);
  IPV4addr[dstNum] = transformIpv4Address(interfaces.GetAddress(interfaces.GetN() - 1));

  if (number_of_links == 255){
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  } else {
    number_of_links++;
  }
}
