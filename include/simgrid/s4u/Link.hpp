/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_LINK_HPP_
#define S4U_LINK_HPP_

#include <xbt/base.h>

#include <unordered_map>

#include "xbt/signal.hpp"

#include "simgrid/link.h"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {
class NetworkAction;
class Action;
};
namespace s4u {
/** @brief A Link represents the network facilities between [hosts](\ref simgrid::s4u::Host) */
class Link {
  friend simgrid::surf::LinkImpl;

private:
  // Links are created from the NetZone, and destroyed by their private implementation when the simulation ends
  explicit Link(surf::LinkImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Link() = default;
  // The private implementation, that never changes
  surf::LinkImpl* const pimpl_;

public:
  /** @brief Retrieve a link from its name */
  static Link* byName(const char* name);

  /** @brief Get da name */
  const char* name();

  /** @brief Get the bandwidth in bytes per second of current Link */
  double bandwidth();

  /** @brief Get the latency in seconds of current Link */
  double latency();

  /** @brief The sharing policy is a @{link e_surf_link_sharing_policy_t::EType} (0: FATPIPE, 1: SHARED, 2: FULLDUPLEX)
   */
  int sharingPolicy();

  /** @brief Check if the Link is used */
  bool isUsed();

  void turnOn();
  void turnOff();

  void* getData();
  void setData(void* d);

  void setStateTrace(tmgr_trace_t trace); /*< setup the trace file with states events (ON or OFF). Trace must contain
                                             boolean values. */
  void setBandwidthTrace(tmgr_trace_t trace); /*< setup the trace file with bandwidth events (peak speed changes due to
                                                  external load). Trace must contain percentages (value between 0 and 1). */
  void setLatencyTrace(tmgr_trace_t trace); /*< setup the trace file with latency events (peak latency changes due to
                                               external load). Trace must contain absolute values */

  /* The signals */
  /** @brief Callback signal fired when a new Link is created */
  static simgrid::xbt::signal<void(surf::LinkImpl*)> onCreation;

  /** @brief Callback signal fired when a Link is destroyed */
  static simgrid::xbt::signal<void(surf::LinkImpl*)> onDestruction;

  /** @brief Callback signal fired when the state of a Link changes (when it is turned on or off) */
  static simgrid::xbt::signal<void(surf::LinkImpl*)> onStateChange;

  /** @brief Callback signal fired when a communication starts */
  static simgrid::xbt::signal<void(surf::NetworkAction*, s4u::Host* src, s4u::Host* dst)> onCommunicate;

  /** @brief Callback signal fired when a communication changes it state (ready/done/cancel) */
  static simgrid::xbt::signal<void(surf::NetworkAction*)> onCommunicationStateChange;
};
}
}

#endif /* SURF_NETWORK_INTERFACE_HPP_ */
