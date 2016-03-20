/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _NS3_SIM_H
#define _NS3_SIM_H

#ifdef __cplusplus

#include <cstdint>

#include "ns3_interface.h"
#include "ns3/core-module.h"
#include "my-point-to-point-helper.h"

#include "ns3/node.h"
#include "ns3/global-route-manager.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/tcp-socket-factory.h"

struct MySocket{
  std::uint32_t bufferedBytes;
  std::uint32_t sentBytes;
  std::uint32_t remaining;
  std::uint32_t totalBytes;
  char finished;
  simgrid::surf::NetworkNS3Action* action;
};

//Simulator s;
class NS3Sim {

private:

public:
  NS3Sim();
  ~NS3Sim();
  void create_flow_NS3(ns3::Ptr<ns3::Node> src,
            ns3::Ptr<ns3::Node> dst,
            std::uint16_t port_number,
            double start,
            const char *addr,
            std::uint32_t TotalBytes,
            simgrid::surf::NetworkNS3Action * action);
  void simulator_start(double min);
};

#endif                          /* __cplusplus */

#endif
