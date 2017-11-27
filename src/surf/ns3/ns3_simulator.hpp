/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NS3_SIMULATOR_HPP
#define NS3_SIMULATOR_HPP

#include <cstdint>

#include "ns3_interface.hpp"

#include <ns3/csma-helper.h>
#include <ns3/global-route-manager.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/tcp-socket-factory.h>

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
