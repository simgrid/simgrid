/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NS3_SIMULATOR_HPP
#define NS3_SIMULATOR_HPP

#include "simgrid/s4u/Host.hpp"
#include "src/surf/network_ns3.hpp"

#include "ns3/wifi-module.h"
#include <ns3/node.h>
#include <ns3/tcp-socket-factory.h>

#include <cstdint>

class XBT_PRIVATE NetPointNs3 {
public:
  static simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> EXTENSION_ID;

  void set_name(std::string name) { name_ = name; }

  explicit NetPointNs3();
  std::string name_;
  ns3::Ptr<ns3::Node> ns3_node_;
  std::string ipv4_address_;
};

XBT_PRIVATE void ns3_initialize(std::string TcpProtocol);
XBT_PRIVATE void ns3_simulator(double max_seconds);
XBT_PRIVATE void ns3_add_direct_route(simgrid::kernel::routing::NetPoint* src, simgrid::kernel::routing::NetPoint* dst,
                                      double bw, double lat, const std::string& link_name,
                                      simgrid::s4u::Link::SharingPolicy policy);

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

XBT_PRIVATE void start_flow(ns3::Ptr<ns3::Socket> sock, const char* to, uint16_t port_number);

static inline std::string transform_socket_ptr(ns3::Ptr<ns3::Socket> local_socket)
{
  std::stringstream sstream;
  sstream << local_socket;
  return sstream.str();
}

class XBT_PRIVATE WifiZone {
public:
  WifiZone(std::string name_, simgrid::s4u::Host* host_, ns3::Ptr<ns3::Node> ap_node_,
           ns3::Ptr<ns3::YansWifiChannel> channel_, int mcs_, int nss_, int network_, int link_);

  const char* get_cname() { return name.c_str(); }
  simgrid::s4u::Host* get_host() { return host; }
  ns3::Ptr<ns3::Node> get_ap_node() { return ap_node; }
  ns3::Ptr<ns3::YansWifiChannel> get_channel() { return channel; }
  int get_mcs() { return mcs; }
  int get_nss() { return nss; }
  int get_network() { return network; }
  int get_link() { return link; }
  int get_n_sta_nodes() { return n_sta_nodes; }

  void set_network(int network_) { network = network_; }
  void set_link(int link_) { link = link_; }
  void add_sta_node() { n_sta_nodes++; }

  static bool is_ap(ns3::Ptr<ns3::Node> node);
  static WifiZone* by_name(std::string name);

private:
  std::string name;
  simgrid::s4u::Host* host;
  ns3::Ptr<ns3::Node> ap_node;
  ns3::Ptr<ns3::YansWifiChannel> channel;
  int mcs;
  int nss;
  int network;
  int link;
  int n_sta_nodes = 0;
  static std::unordered_map<std::string, WifiZone*> wifi_zones;
};

#endif
