/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <unordered_set>

#include "xbt/config.hpp"
#include "xbt/string.hpp"
#include "xbt/utility.hpp"

#include "ns3/core-module.h"
#include "ns3/node.h"

#include "network_ns3.hpp"
#include "ns3/ns3_simulator.hpp"

#include "src/instr/instr_private.hpp" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals
#include "src/kernel/routing/NetPoint.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ns3, surf, "Logging specific to the SURF network NS3 module");

std::vector<std::string> IPV4addr;

/*****************
 * Crude globals *
 *****************/

extern std::map<std::string, SgFlow*> flowFromSock;

static ns3::InternetStackHelper stack;
static ns3::NodeContainer nodes;
static ns3::NodeContainer Cluster_nodes;
static ns3::Ipv4InterfaceContainer interfaces;

static int number_of_nodes = 0;
static int number_of_clusters_nodes = 0;
static int number_of_links = 1;
static int number_of_networks = 1;
static int port_number = 1025; //Port number is limited from 1025 to 65 000

simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> NetPointNs3::EXTENSION_ID;

NetPointNs3::NetPointNs3()
{
  ns3::Ptr<ns3::Node> node = ns3::CreateObject<ns3::Node>(0);
  stack.Install(node);
  nodes.Add(node);
  node_num = number_of_nodes++;
}

/*************
 * Callbacks *
 *************/

static void clusterCreation_cb(ClusterCreationArgs* cluster)
{
  for (int const& i : *cluster->radicals) {
    // Routers don't create a router on the other end of the private link by themselves.
    // We just need this router to be given an ID so we create a temporary NetPointNS3 so that it gets one
    NetPointNs3* host_dst = new NetPointNs3();

    // Create private link
    std::string host_id   = cluster->prefix + std::to_string(i) + cluster->suffix;
    NetPointNs3* host_src = sg_host_by_name(host_id.c_str())->pimpl_netpoint->extension<NetPointNs3>();
    xbt_assert(host_src, "Cannot find a NS3 host of name %s", host_id.c_str());

    // Any NS3 route is symmetrical
    ns3_add_link(host_src, host_dst, cluster->bw, cluster->lat);

    delete host_dst;
  }

  //Create link backbone
  ns3_add_cluster(cluster->id.c_str(), cluster->bb_bw, cluster->bb_lat);
}

static void routeCreation_cb(bool symmetrical, simgrid::kernel::routing::NetPoint* src,
                             simgrid::kernel::routing::NetPoint* dst, simgrid::kernel::routing::NetPoint* gw_src,
                             simgrid::kernel::routing::NetPoint* gw_dst,
                             std::vector<simgrid::surf::LinkImpl*>& link_list)
{
  if (link_list.size() == 1) {
    simgrid::surf::LinkNS3* link = static_cast<simgrid::surf::LinkNS3*>(link_list[0]);

    XBT_DEBUG("Route from '%s' to '%s' with link '%s' %s", src->getCname(), dst->getCname(), link->getCname(),
              (symmetrical ? "(symmetrical)" : "(not symmetrical)"));

    //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
    XBT_DEBUG("\tLink (%s) bw:%fbps lat:%fs", link->getCname(), link->bandwidth(), link->latency());

    // create link ns3
    NetPointNs3* host_src = src->extension<NetPointNs3>();
    NetPointNs3* host_dst = dst->extension<NetPointNs3>();

    xbt_assert(host_src != nullptr, "Network element %s does not seem to be NS3-ready", src->getCname());
    xbt_assert(host_dst != nullptr, "Network element %s does not seem to be NS3-ready", dst->getCname());

    ns3_add_link(host_src, host_dst, link->bandwidth(), link->latency());
  } else {
    static bool warned_about_long_routes = false;

    if (not warned_about_long_routes)
      XBT_WARN("Ignoring a route between %s and %s of length %zu: Only routes of length 1 are considered with NS3.\n"
               "WARNING: You can ignore this warning if your hosts can still communicate when only considering routes "
               "of length 1.\n"
               "WARNING: Remove long routes to avoid this harmless message; subsequent long routes will be silently "
               "ignored.",
               src->getCname(), dst->getCname(), link_list.size());
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
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkNS3Model();
  all_existing_models->push_back(surf_network_model);
}

static simgrid::config::Flag<std::string>
    ns3_tcp_model("ns3/TcpModel", "The ns3 tcp model can be : NewReno or Reno or Tahoe", "default");

namespace simgrid {
namespace surf {

NetworkNS3Model::NetworkNS3Model() : NetworkModel() {
  NetPointNs3::EXTENSION_ID = simgrid::kernel::routing::NetPoint::extension_create<NetPointNs3>();

  ns3_initialize(ns3_tcp_model.get().c_str());

  simgrid::kernel::routing::NetPoint::onCreation.connect([](simgrid::kernel::routing::NetPoint* pt) {
    pt->extension_set<NetPointNs3>(new NetPointNs3());
    XBT_VERB("SimGrid's %s is known as node %d within NS3", pt->getCname(), pt->extension<NetPointNs3>()->node_num);
  });
  simgrid::surf::on_cluster.connect(&clusterCreation_cb);
  simgrid::s4u::onPlatformCreated.connect(&postparse_cb);
  simgrid::s4u::NetZone::onRouteCreation.connect(&routeCreation_cb);
}

NetworkNS3Model::~NetworkNS3Model() {
  IPV4addr.clear();
}

LinkImpl* NetworkNS3Model::createLink(const std::string& name, double bandwidth, double latency,
                                      e_surf_link_sharing_policy_t policy)
{
  return new LinkNS3(this, name, bandwidth, latency);
}

Action* NetworkNS3Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  return new NetworkNS3Action(this, size, src, dst);
}

double NetworkNS3Model::nextOccuringEvent(double now)
{
  double time_to_next_flow_completion;
  XBT_DEBUG("ns3_next_occuring_event");

  //get the first relevant value from the running_actions list
  if (not getRunningActionSet()->size() || now == 0.0)
    return -1.0;
  else
    do {
      ns3_simulator(now);
      time_to_next_flow_completion = ns3::Simulator::Now().GetSeconds() - surf_get_clock();
    } while(double_equals(time_to_next_flow_completion, 0, sg_surf_precision));

  XBT_DEBUG("min       : %f", now);
  XBT_DEBUG("ns3  time : %f", ns3::Simulator::Now().GetSeconds());
  XBT_DEBUG("surf time : %f", surf_get_clock());
  XBT_DEBUG("Next completion %f :", time_to_next_flow_completion);

  return time_to_next_flow_completion;
}

void NetworkNS3Model::updateActionsState(double now, double delta)
{
  static std::vector<std::string> socket_to_destroy;

  /* If there are no running flows, advance the NS3 simulator and return */
  if (getRunningActionSet()->empty()) {

    while(double_positive(now - ns3::Simulator::Now().GetSeconds(), sg_surf_precision))
      ns3_simulator(now-ns3::Simulator::Now().GetSeconds());

    return;
  }

  std::string ns3Socket;
  for (auto elm : flowFromSock) {
    ns3Socket                 = elm.first;
    SgFlow* sgFlow            = elm.second;
    NetworkNS3Action * action = sgFlow->action_;
    XBT_DEBUG("Processing socket %p (action %p)",sgFlow,action);
    action->setRemains(action->getCost() - sgFlow->sentBytes_);

    if (TRACE_is_enabled() &&
        action->getState() == Action::State::running){
      double data_delta_sent = sgFlow->sentBytes_ - action->lastSent_;

      std::vector<LinkImpl*> route = std::vector<LinkImpl*>();

      action->src_->routeTo(action->dst_, route, nullptr);
      for (auto const& link : route)
        TRACE_surf_link_set_utilization(link->getCname(), action->getCategory(), (data_delta_sent) / delta, now - delta,
                                        delta);

      action->lastSent_ = sgFlow->sentBytes_;
    }

    if(sgFlow->finished_){
      socket_to_destroy.push_back(ns3Socket);
      XBT_DEBUG("Destroy socket %p of action %p", ns3Socket, action);
      action->finish(Action::State::done);
    }
  }

  while (not socket_to_destroy.empty()) {
    ns3Socket = socket_to_destroy.back();
    socket_to_destroy.pop_back();
    SgFlow* flow = flowFromSock.at(ns3Socket);
    if (XBT_LOG_ISENABLED(ns3, xbt_log_priority_debug)) {
      XBT_DEBUG ("Removing socket %p of action %p", ns3Socket, flow->action_);
    }
    delete flow;
    flowFromSock.erase(ns3Socket);
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

  s4u::Link::onCreation(this->piface_);
}

LinkNS3::~LinkNS3() = default;

void LinkNS3::apply_event(tmgr_trace_event_t event, double value)
{
  THROW_UNIMPLEMENTED;
}
void LinkNS3::setBandwidthTrace(tmgr_trace_t trace) {
  xbt_die("The NS3 network model doesn't support bandwidth traces");
}
void LinkNS3::setLatencyTrace(tmgr_trace_t trace) {
  xbt_die("The NS3 network model doesn't support latency traces");
}

/**********
 * Action *
 **********/

NetworkNS3Action::NetworkNS3Action(Model* model, double size, s4u::Host* src, s4u::Host* dst)
    : NetworkAction(model, size, false)
{
  XBT_DEBUG("Communicate from %s to %s", src->getCname(), dst->getCname());

  src_ = src;
  dst_ = dst;
  ns3_create_flow(src, dst, size, this);

  s4u::Link::onCommunicate(this, src, dst);
}

void NetworkNS3Action::suspend() {
  THROW_UNIMPLEMENTED;
}

void NetworkNS3Action::resume() {
  THROW_UNIMPLEMENTED;
}

std::list<LinkImpl*> NetworkNS3Action::links()
{
  THROW_UNIMPLEMENTED;
}

/* Test whether a flow is suspended */
bool NetworkNS3Action::isSuspended()
{
  return false;
}

int NetworkNS3Action::unref()
{
  refcount_--;
  if (not refcount_) {
    if (action_hook.is_linked())
      simgrid::xbt::intrusive_erase(*stateSet_, *this);
    XBT_DEBUG ("Removing action %p", this);
    delete this;
    return 1;
  }
  return 0;
}

}
}

void ns3_simulator(double maxSeconds)
{
  if (maxSeconds > 0.0) // If there is a maximum amount of time to run
    ns3::Simulator::Stop(ns3::Seconds(maxSeconds));
  XBT_DEBUG("Start simulator for at most %fs (current time: %f)", maxSeconds, surf_get_clock());
  ns3::Simulator::Run ();
}

void ns3_create_flow(simgrid::s4u::Host* src, simgrid::s4u::Host* dst,
    u_int32_t TotalBytes,
    simgrid::surf::NetworkNS3Action* action) {
  unsigned int node1 = src->pimpl_netpoint->extension<NetPointNs3>()->node_num;
  unsigned int node2 = dst->pimpl_netpoint->extension<NetPointNs3>()->node_num;

  ns3::Ptr<ns3::Node> src_node = nodes.Get(node1);
  ns3::Ptr<ns3::Node> dst_node = nodes.Get(node2);

  xbt_assert(node2 < IPV4addr.size(), "Element %s is unknown to NS3. Is it connected to any one-hop link?",
             dst->pimpl_netpoint->getCname());
  std::string& addr = IPV4addr[node2];
  xbt_assert(not addr.empty(), "Element %s is unknown to NS3. Is it connected to any one-hop link?",
             dst->pimpl_netpoint->getCname());

  XBT_DEBUG("ns3_create_flow %u Bytes from %u to %u with Interface %s", TotalBytes, node1, node2, addr.c_str());
  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", ns3::InetSocketAddress (ns3::Ipv4Address::GetAny(), port_number));
  sink.Install (dst_node);

  ns3::Ptr<ns3::Socket> sock = ns3::Socket::CreateSocket(src_node, ns3::TcpSocketFactory::GetTypeId());

  flowFromSock.insert({transformSocketPtr(sock), new SgFlow(TotalBytes, action)});

  sock->Bind(ns3::InetSocketAddress(port_number));

  ns3::Simulator::ScheduleNow(&StartFlow, sock, addr.c_str(), port_number);

  port_number++;
  xbt_assert(port_number <= 65000, "Too many connections! Port number is saturated.");
}

// initialize the NS3 interface and environment
void ns3_initialize(const char* TcpProtocol){
//  tcpModel are:
//  "ns3::TcpNewReno"
//  "ns3::TcpReno"
//  "ns3::TcpTahoe"

  ns3::Config::SetDefault ("ns3::TcpSocket::SegmentSize", ns3::UintegerValue (1000));
  ns3::Config::SetDefault ("ns3::TcpSocket::DelAckCount", ns3::UintegerValue (1));
  ns3::Config::SetDefault ("ns3::TcpSocketBase::Timestamp", ns3::BooleanValue (false));

  if (not strcmp(TcpProtocol, "default"))
    return;

  if (not strcmp(TcpProtocol, "Reno")) {
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpReno"));
    return;
  }
  if (not strcmp(TcpProtocol, "NewReno")) {
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpNewReno"));
    return;
  }
  if (not strcmp(TcpProtocol, "Tahoe")) {
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpTahoe"));
    return;
  }

  xbt_die("The ns3/TcpModel must be: NewReno or Reno or Tahoe");
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

  xbt_assert(Nodes.GetN() <= 65000, "Cluster with NS3 is limited to 65000 nodes");
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", ns3::DataRateValue(ns3::DataRate(bw * 8))); // NS3 takes bps, but we provide Bps
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

  ns3::Ptr<ns3::Node> a = nodes.Get(srcNum);
  ns3::Ptr<ns3::Node> b = nodes.Get(dstNum);

  XBT_DEBUG("\tAdd PTP from %d to %d bw:'%f Bps' lat:'%fs'", srcNum, dstNum, bw, lat);
  pointToPoint.SetChannelAttribute("DataRate",
                                   ns3::DataRateValue(ns3::DataRate(bw * 8))); // NS3 takes bps, but we provide Bps
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
