/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unordered_set>

#include <xbt/config.hpp>

#include "ns3/core-module.h"
#include "ns3/node.h"

#include "ns3/ns3_interface.h"
#include "ns3/ns3_simulator.h"
#include "network_ns3.hpp"

#include "src/surf/HostImpl.hpp"
#include "src/surf/surf_private.h"
#include "simgrid/sg_config.h"
#include "src/instr/instr_private.h" // TRACE_is_enabled(). FIXME: remove by subscribing tracing to the surf signals

#include "simgrid/s4u/As.hpp"
#include "simgrid/s4u/engine.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ns3, surf, "Logging specific to the SURF network NS3 module");

xbt_dynar_t IPV4addr = xbt_dynar_new(sizeof(char*),free);

/*****************
 * Crude globals *
 *****************/

extern xbt_dict_t flowFromSock;

static ns3::InternetStackHelper stack;
static ns3::NodeContainer nodes;
static ns3::NodeContainer Cluster_nodes;
static ns3::Ipv4InterfaceContainer interfaces;

static int number_of_nodes = 0;
static int number_of_clusters_nodes = 0;
static int number_of_links = 1;
static int number_of_networks = 1;
static int port_number = 1025; //Port number is limited from 1025 to 65 000

HostNs3::HostNs3()
{
  ns3::Ptr<ns3::Node> node = ns3::CreateObject<ns3::Node>(0);
  stack.Install(node);
  nodes.Add(node);
  node_num = number_of_nodes++;
}

/*************
 * Callbacks *
 *************/

static void ns3_add_host(simgrid::s4u::Host& host)
{
  host.extension_set<HostNs3>(new HostNs3());
}

static void ns3_add_netcard(simgrid::kernel::routing::NetCard* netcard)
{
  xbt_lib_set(as_router_lib, netcard->name().c_str(), NS3_ASR_LEVEL, new HostNs3());
}

#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area
static void parse_ns3_add_cluster(sg_platf_cluster_cbarg_t cluster)
{
  char* lat = bprintf("%fs", cluster->lat);
  char* bw  = bprintf("%fBps", cluster->bw);

  for (int i : *cluster->radicals) {
    char* router_id = bprintf("router_%s%d%s", cluster->prefix, i, cluster->suffix);

    simgrid::s4u::Host* router = new simgrid::s4u::Host(router_id);
    ns3_add_host(*router);

    // Create private link
    char* host_id = bprintf("%s%d%s", cluster->prefix, i, cluster->suffix);
    HostNs3* host_src = ns3_find_host(host_id);
    HostNs3* host_dst = router->extension<HostNs3>();

    xbt_assert(host_src && host_dst, "\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

    ns3_add_link(host_src->node_num, host_dst->node_num, bw,lat);

    free(router_id);
    free(host_id);
  }
  xbt_free(lat);
  xbt_free(bw);

  //Create link backbone
  lat = bprintf("%fs", cluster->bb_lat);
  bw =  bprintf("%fBps", cluster->bb_bw);
  ns3_add_cluster(cluster->id, bw, lat);
  xbt_free(lat);
  xbt_free(bw);
}

/* Create the ns3 topology based on routing strategy */
static void create_ns3_topology(void)
{
  XBT_DEBUG("Starting topology generation");

  xbt_dynar_shrink(IPV4addr,0);

  //get the onelinks from the parsed platform
  std::vector<simgrid::kernel::routing::Onelink*> onelink_routes;
  static_cast<simgrid::kernel::routing::AsImpl*>(simgrid::s4u::Engine::instance()->rootAs())
      ->getOneLinkRoutes(&onelink_routes);

  std::unordered_set<simgrid::surf::LinkNS3*> already_seen = std::unordered_set<simgrid::surf::LinkNS3*>();

  XBT_DEBUG("There is %ld one-link routes", onelink_routes.size());
  for (simgrid::kernel::routing::Onelink* onelink : onelink_routes) {
    const char* src              = onelink->src_->name().c_str();
    const char* dst              = onelink->dst_->name().c_str();
    simgrid::surf::LinkNS3 *link = static_cast<simgrid::surf::LinkNS3 *>(onelink->link_);

    if (strcmp(src,dst) && (already_seen.find(link) == already_seen.end())) {
      already_seen.insert(link);
      XBT_DEBUG("Route from '%s' to '%s' with link '%s'", src, dst, link->getName());
      char* link_bdw = bprintf("%fBps", link->bandwidth());
      char* link_lat = bprintf("%fs", link->latency());

      //   XBT_DEBUG("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
      XBT_DEBUG("\tLink (%s) bdw:%s lat:%s", link->getName(), link_bdw, link_lat);

      //create link ns3
      HostNs3* host_src = ns3_find_host(src);
      if (!host_src)
        host_src        = static_cast<HostNs3*>(xbt_lib_get_or_null(as_router_lib, src, NS3_ASR_LEVEL));
      HostNs3* host_dst = ns3_find_host(dst);
      if(!host_dst)
        host_dst = static_cast<HostNs3*>(xbt_lib_get_or_null(as_router_lib, dst, NS3_ASR_LEVEL));

      if (!host_src || !host_dst)
          xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

      ns3_add_link(host_src->node_num, host_dst->node_num, link_bdw, link_lat);

      xbt_free(link_bdw);
      xbt_free(link_lat);
    }
  }

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

static simgrid::config::Flag<std::string> ns3_tcp_model("ns3/TcpModel",
  "The ns3 tcp model can be : NewReno or Reno or Tahoe",
  "default");

simgrid::xbt::Extension<simgrid::s4u::Host, HostNs3> HostNs3::EXTENSION_ID;

namespace simgrid {
namespace surf {

NetworkNS3Model::NetworkNS3Model() : NetworkModel() {
  ns3_initialize(ns3_tcp_model.get().c_str());

  simgrid::s4u::Host::onCreation.connect(ns3_add_host);
  simgrid::kernel::routing::netcardCreatedCallbacks.connect(ns3_add_netcard);
  simgrid::surf::on_cluster.connect (&parse_ns3_add_cluster);
  simgrid::surf::on_postparse.connect(&create_ns3_topology);

  HostNs3::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostNs3>();

  NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib, xbt_free_f);

  LogComponentEnable("UdpEchoClientApplication", ns3::LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", ns3::LOG_LEVEL_INFO);
}

NetworkNS3Model::~NetworkNS3Model() {
  xbt_dynar_free_container(&IPV4addr);
  xbt_dict_free(&flowFromSock);
}

Link* NetworkNS3Model::createLink(const char* name, double bandwidth, double latency,
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
  if (!getRunningActionSet()->size() || now == 0.0)
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
  static xbt_dynar_t socket_to_destroy = xbt_dynar_new(sizeof(char*),nullptr);

  /* If there are no running flows, advance the NS3 simulator and return */
  if (getRunningActionSet()->empty()) {

    while(double_positive(now - ns3::Simulator::Now().GetSeconds(), sg_surf_precision))
      ns3_simulator(now-ns3::Simulator::Now().GetSeconds());

    return;
  }

  xbt_dict_cursor_t cursor = nullptr;
  char *ns3Socket;
  SgFlow *sgFlow;
  xbt_dict_foreach(flowFromSock,cursor,ns3Socket,sgFlow){
    NetworkNS3Action * action = sgFlow->action_;
    XBT_DEBUG("Processing socket %p (action %p)",sgFlow,action);
    action->setRemains(action->getCost() - sgFlow->sentBytes_);

    if (TRACE_is_enabled() &&
        action->getState() == Action::State::running){
      double data_delta_sent = sgFlow->sentBytes_ - action->lastSent_;

      std::vector<Link*> route = std::vector<Link*>();

      routing_platf->getRouteAndLatency(action->src_->pimpl_netcard, action->dst_->pimpl_netcard, &route, nullptr);
      for (auto link : route)
        TRACE_surf_link_set_utilization (link->getName(), action->getCategory(), (data_delta_sent)/delta, now-delta, delta);

      action->lastSent_ = sgFlow->sentBytes_;
    }

    if(sgFlow->finished_){
      xbt_dynar_push(socket_to_destroy,&ns3Socket);
      XBT_DEBUG("Destroy socket %p of action %p", ns3Socket, action);
      action->finish();
      action->setState(Action::State::done);
    }
  }

  while (!xbt_dynar_is_empty(socket_to_destroy)){
    xbt_dynar_pop(socket_to_destroy,&ns3Socket);

    if (XBT_LOG_ISENABLED(ns3, xbt_log_priority_debug)) {
      SgFlow *flow = (SgFlow*)xbt_dict_get (flowFromSock, ns3Socket);
      XBT_DEBUG ("Removing socket %p of action %p", ns3Socket, flow->action_);
    }
    xbt_dict_remove(flowFromSock, ns3Socket);
  }
}

/************
 * Resource *
 ************/

LinkNS3::LinkNS3(NetworkNS3Model* model, const char* name, double bandwidth, double latency)
    : Link(model, name, nullptr)
{
  bandwidth_.peak = bandwidth;
  latency_.peak   = latency;

  Link::onCreation(this);
}

LinkNS3::~LinkNS3()
{
}

void LinkNS3::apply_event(tmgr_trace_iterator_t event, double value)
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
  XBT_DEBUG("Communicate from %s to %s", src->name().c_str(), dst->name().c_str());

  src_ = src;
  dst_ = dst;
  ns3_create_flow(src->name().c_str(), dst->name().c_str(), surf_get_clock(), size, this);

  Link::onCommunicate(this, src, dst);
}

void NetworkNS3Action::suspend() {
  THROW_UNIMPLEMENTED;
}

void NetworkNS3Action::resume() {
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
  if (!refcount_) {
    if (action_hook.is_linked())
      stateSet_->erase(stateSet_->iterator_to(*this));
    XBT_DEBUG ("Removing action %p", this);
    delete this;
    return 1;
  }
  return 0;
}

}
}

void ns3_simulator(double maxSeconds){
  if (maxSeconds > 0.0) // If there is a maximum amount of time to run
    ns3::Simulator::Stop(ns3::Seconds(maxSeconds));
  XBT_DEBUG("Start simulator for at most %fs",maxSeconds);
  ns3::Simulator::Run ();
}

void ns3_create_flow(const char* a,const char *b,double startTime,u_int32_t TotalBytes,simgrid::surf::NetworkNS3Action * action)
{
  int node1 = ns3_find_host(a)->node_num;
  int node2 = ns3_find_host(b)->node_num;

  ns3::Ptr<ns3::Node> src_node = nodes.Get(node1);
  ns3::Ptr<ns3::Node> dst_node = nodes.Get(node2);

  char* addr = (char*)xbt_dynar_get_as(IPV4addr,node2,char*);

  XBT_DEBUG("ns3_create_flow %d Bytes from %d to %d with Interface %s",TotalBytes, node1, node2,addr);
  ns3::PacketSinkHelper sink("ns3::TcpSocketFactory", ns3::InetSocketAddress (ns3::Ipv4Address::GetAny(), port_number));
  sink.Install (dst_node);

  ns3::Ptr<ns3::Socket> sock = ns3::Socket::CreateSocket (src_node, ns3::TcpSocketFactory::GetTypeId());

  xbt_dict_set(flowFromSock, transformSocketPtr(sock), new SgFlow(TotalBytes, action), nullptr);

  sock->Bind(ns3::InetSocketAddress(port_number));
  XBT_DEBUG("Create flow starting to %fs + %fs = %fs",
      startTime-ns3::Simulator::Now().GetSeconds(), ns3::Simulator::Now().GetSeconds(), startTime);

  ns3::Simulator::Schedule (ns3::Seconds(startTime-ns3::Simulator::Now().GetSeconds()),
      &StartFlow, sock, addr, port_number);

  port_number++;
  xbt_assert(port_number <= 65000, "Too many connections! Port number is saturated.");
}

// initialize the NS3 interface and environment
void ns3_initialize(const char* TcpProtocol){
//  tcpModel are:
//  "ns3::TcpNewReno"
//  "ns3::TcpReno"
//  "ns3::TcpTahoe"

  ns3::Config::SetDefault ("ns3::TcpSocket::SegmentSize", ns3::UintegerValue (1024)); // 1024-byte packet for easier reading
  ns3::Config::SetDefault ("ns3::TcpSocket::DelAckCount", ns3::UintegerValue (1));

  if (!strcmp(TcpProtocol,"default"))
    return;

  if (!strcmp(TcpProtocol,"Reno")) {
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpReno"));
    return;
  }
  if (!strcmp(TcpProtocol,"NewReno")) {
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpNewReno"));
    return;
  }
  if(!strcmp(TcpProtocol,"Tahoe")){
    XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
    ns3::Config::SetDefault ("ns3::TcpL4Protocol::SocketType", ns3::StringValue("ns3::TcpTahoe"));
    return;
  }

  xbt_die("The ns3/TcpModel must be : NewReno or Reno or Tahoe");
}

void ns3_add_cluster(const char* id, char* bw, char* lat)
{
  ns3::NodeContainer Nodes;

  for (unsigned int i = number_of_clusters_nodes; i < Cluster_nodes.GetN(); i++) {
    Nodes.Add(Cluster_nodes.Get(i));
    XBT_DEBUG("Add node %d to cluster",i);
  }
  number_of_clusters_nodes = Cluster_nodes.GetN();

  XBT_DEBUG("Add router %d to cluster",nodes.GetN()-Nodes.GetN()-1);
  Nodes.Add(nodes.Get(nodes.GetN()-Nodes.GetN()-1));

  xbt_assert(Nodes.GetN() <= 65000, "Cluster with NS3 is limited to 65000 nodes");
  ns3::CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", ns3::StringValue (bw));
  csma.SetChannelAttribute ("Delay", ns3::StringValue (lat));
  ns3::NetDeviceContainer devices = csma.Install (Nodes);
  XBT_DEBUG("Create CSMA");

  char * adr = bprintf("%d.%d.0.0",number_of_networks,number_of_links);
  XBT_DEBUG("Assign IP Addresses %s to CSMA.",adr);
  ns3::Ipv4AddressHelper ipv4;
  ipv4.SetBase (adr, "255.255.0.0");
  free(adr);
  interfaces.Add(ipv4.Assign (devices));

  if(number_of_links == 255){
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  }else{
    number_of_links++;
  }
  XBT_DEBUG("Number of nodes in Cluster_nodes: %d",Cluster_nodes.GetN());
}

static char* transformIpv4Address (ns3::Ipv4Address from){
  std::stringstream sstream;
  sstream << from ;
  std::string s = sstream.str();
  return bprintf("%s",s.c_str());
}

void ns3_add_link(int src, int dst, char *bw, char *lat)
{
  ns3::PointToPointHelper pointToPoint;

  ns3::NetDeviceContainer netA;
  ns3::Ipv4AddressHelper address;

  ns3::Ptr<ns3::Node> a = nodes.Get(src);
  ns3::Ptr<ns3::Node> b = nodes.Get(dst);

  XBT_DEBUG("\tAdd PTP from %d to %d bw:'%s' lat:'%s'",src,dst,bw,lat);
  pointToPoint.SetDeviceAttribute ("DataRate", ns3::StringValue (bw));
  pointToPoint.SetChannelAttribute ("Delay", ns3::StringValue (lat));

  netA.Add(pointToPoint.Install (a, b));

  char * adr = bprintf("%d.%d.0.0",number_of_networks,number_of_links);
  address.SetBase (adr, "255.255.0.0");
  XBT_DEBUG("\tInterface stack '%s'",adr);
  free(adr);
  interfaces.Add(address.Assign (netA));

  char *tmp = transformIpv4Address(interfaces.GetAddress(interfaces.GetN()-2));
  xbt_dynar_set_as(IPV4addr,src,char*,tmp);
  XBT_DEBUG("Have write '%s' for Node '%d'",(char*)xbt_dynar_get_as(IPV4addr,src,char*),src);

  tmp = transformIpv4Address(interfaces.GetAddress(interfaces.GetN()-1));
  xbt_dynar_set_as(IPV4addr,dst,char*,tmp);
  XBT_DEBUG("Have write '%s' for Node '%d'",(char*)xbt_dynar_get_as(IPV4addr,dst,char*),dst);

  if (number_of_links == 255){
    xbt_assert(number_of_networks < 255, "Number of links and networks exceed 255*255");
    number_of_links = 1;
    number_of_networks++;
  } else {
    number_of_links++;
  }
}
