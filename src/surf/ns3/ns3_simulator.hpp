/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NS3_SIMULATOR_HPP
#define NS3_SIMULATOR_HPP

#include "simgrid/s4u/Host.hpp"

#include <ns3/node.h>
#include <ns3/tcp-socket-factory.h>

#include <cstdint>

namespace simgrid {
namespace surf {
class NetworkNS3Action;
}
} // namespace simgrid

class NetPointNs3 {
public:
  static simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> EXTENSION_ID;

  explicit NetPointNs3();
  int node_num;
  ns3::Ptr<ns3::Node> ns3Node_;
};

XBT_PUBLIC void ns3_initialize(std::string TcpProtocol);
extern "C" {
XBT_PUBLIC void ns3_simulator(double maxSeconds);
XBT_PUBLIC void ns3_add_link(NetPointNs3* src, NetPointNs3* dst, double bw, double lat);
XBT_PUBLIC void ns3_add_cluster(const char* id, double bw, double lat);
}

class XBT_PRIVATE SgFlow {
public:
  SgFlow(uint32_t totalBytes, simgrid::surf::NetworkNS3Action* action);

  // private:
  std::uint32_t bufferedBytes_ = 0;
  std::uint32_t sentBytes_     = 0;
  std::uint32_t remaining_;
  std::uint32_t totalBytes_;
  bool finished_ = false;
  simgrid::surf::NetworkNS3Action* action_;
};

void StartFlow(ns3::Ptr<ns3::Socket> sock, const char* to, uint16_t port_number);

static inline std::string transformSocketPtr(ns3::Ptr<ns3::Socket> localSocket)
{
  std::stringstream sstream;
  sstream << localSocket;
  return sstream.str();
}

#endif
