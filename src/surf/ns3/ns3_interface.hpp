/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NS3_INTERFACE_HPP
#define NS3_INTERFACE_HPP

#include "simgrid/s4u/Host.hpp"

namespace simgrid {
namespace surf {
class NetworkNS3Action;
}
}

class NetPointNs3 {
public:
  static simgrid::xbt::Extension<simgrid::kernel::routing::NetPoint, NetPointNs3> EXTENSION_ID;

  explicit NetPointNs3();
  int node_num;
};

extern "C" {

XBT_PUBLIC(void) ns3_initialize(const char* TcpProtocol);
XBT_PUBLIC(void)
ns3_create_flow(sg_host_t src, sg_host_t dst, u_int32_t TotalBytes, simgrid::surf::NetworkNS3Action* action);
XBT_PUBLIC(void) ns3_simulator(double maxSeconds);
XBT_PUBLIC(void*) ns3_add_router(const char* id);
XBT_PUBLIC(void) ns3_add_link(NetPointNs3* src, NetPointNs3* dst, double bw, double lat);
XBT_PUBLIC(void) ns3_add_cluster(const char* id, double bw, double lat);
}

#endif
