/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NS3_SIMULATOR_HPP
#define NS3_SIMULATOR_HPP

#include "simgrid/s4u/Host.hpp"
#include "src/surf/network_ns3.hpp"

#include <ns3/node.h>
#include <ns3/tcp-socket-factory.h>
#include "ns3/wifi-module.h"

#include <cstdint>

class NetPointNs3 {
public:
  static simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> EXTENSION_ID;

  explicit NetPointNs3();
  int node_num;
  ns3::Ptr<ns3::Node> ns3_node_;
};

XBT_PUBLIC void ns3_initialize(std::string TcpProtocol);
XBT_PUBLIC void ns3_simulator(double max_seconds);
XBT_PUBLIC void ns3_add_direct_route(NetPointNs3* src, NetPointNs3* dst, double bw, double lat, std::string link_name,
                                     simgrid::s4u::Link::SharingPolicy policy);
XBT_PUBLIC void ns3_add_cluster(const char* id, double bw, double lat);

class XBT_PRIVATE SgFlow {
public:
  SgFlow(uint32_t total_bytes, simgrid::kernel::resource::NetworkNS3Action* action);

  // private:
  std::uint32_t buffered_bytes_ = 0;
  std::uint32_t sent_bytes_     = 0;
  std::uint32_t remaining_;
  std::uint32_t total_bytes_;
  bool finished_ = false;
  simgrid::kernel::resource::NetworkNS3Action* action_;
};

void start_flow(ns3::Ptr<ns3::Socket> sock, const char* to, uint16_t port_number);

static inline std::string transform_socket_ptr(ns3::Ptr<ns3::Socket> local_socket)
{
  std::stringstream sstream;
  sstream << local_socket;
  return sstream.str();
}

class XBT_PRIVATE WifiZone {
public:
  WifiZone(std::string name_, simgrid::s4u::Host* host_, ns3::Ptr<ns3::Node> ap_node_,
           ns3::Ptr<ns3::YansWifiChannel> channel_, int network_, int link_);

  const char* get_cname();
  simgrid::s4u::Host* get_host();
  ns3::Ptr<ns3::Node> get_ap_node();
  ns3::Ptr<ns3::YansWifiChannel> get_channel();
  int get_network();
  int get_link();
  int get_n_sta_nodes();

  void set_ap_node(ns3::Ptr<ns3::Node> ap_node_);
  void set_network(int network_);
  void set_link(int link_);
  void add_sta_node();
  static bool is_ap(ns3::Ptr<ns3::Node> ap);

  static WifiZone* by_name(std::string name);

private:
  std::string name;
  simgrid::s4u::Host* host;
  ns3::Ptr<ns3::Node> ap_node;
  ns3::Ptr<ns3::YansWifiChannel> channel;
  int network;
  int link;
  int n_sta_nodes;
  static std::unordered_map<std::string, WifiZone*> wifi_zones;
};

#endif
