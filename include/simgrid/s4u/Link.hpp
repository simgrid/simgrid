/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_LINK_HPP_
#define S4U_LINK_HPP_

#include <simgrid/link.h>
#include <string>
#include <unordered_map>
#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace s4u {
/** @brief A Link represents the network facilities between [hosts](\ref simgrid::s4u::Host) */
class XBT_PUBLIC Link : public simgrid::xbt::Extendable<Link> {
  friend simgrid::kernel::resource::LinkImpl;

  // Links are created from the NetZone, and destroyed by their private implementation when the simulation ends
  explicit Link(kernel::resource::LinkImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Link() = default;
  // The private implementation, that never changes
  kernel::resource::LinkImpl* const pimpl_;

public:
  enum class SharingPolicy { SPLITDUPLEX = 2, SHARED = 1, FATPIPE = 0 };

  /** @brief Retrieve a link from its name */
  static Link* by_name(const char* name);

  /** @brief Retrieves the name of that link as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that link as a C string */
  const char* get_cname() const;

  XBT_ATTRIB_DEPRECATED_v323("Please use Link::by_name()") static Link* byName(const char* name) { return by_name(name); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_name()") const std::string& getName() const { return get_name(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_cname()") const char* getCname() const { return get_cname(); }

  /** @brief Get the bandwidth in bytes per second of current Link */
  double bandwidth();

  /** @brief Get the latency in seconds of current Link */
  double latency();

  /** @brief The sharing policy is a @{link e_surf_link_sharing_policy_t::EType} (0: FATPIPE, 1: SHARED, 2: SPLITDUPLEX)
   */
  SharingPolicy sharingPolicy();

  /** @brief Returns the current load (in flops per second) */
  double getUsage();

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

  const char* getProperty(const char* key);
  void setProperty(std::string key, std::string value);

  /* The signals */
  /** @brief Callback signal fired when a new Link is created */
  static simgrid::xbt::signal<void(s4u::Link&)> on_creation;

  /** @brief Callback signal fired when a Link is destroyed */
  static simgrid::xbt::signal<void(s4u::Link&)> on_destruction;

  /** @brief Callback signal fired when the state of a Link changes (when it is turned on or off) */
  static simgrid::xbt::signal<void(s4u::Link&)> on_state_change;

  /** @brief Callback signal fired when the bandwidth of a Link changes */
  static simgrid::xbt::signal<void(s4u::Link&)> on_bandwidth_change;

  /** @brief Callback signal fired when a communication starts */
  static simgrid::xbt::signal<void(kernel::resource::NetworkAction*, s4u::Host* src, s4u::Host* dst)> on_communicate;

  /** @brief Callback signal fired when a communication changes it state (ready/done/cancel) */
  static simgrid::xbt::signal<void(kernel::resource::NetworkAction*)> on_communication_state_change;

  XBT_ATTRIB_DEPRECATED_v321("Use get_cname(): v3.21 will turn this warning into an error.") const char* name();
};
}
}

#endif /* SURF_NETWORK_INTERFACE_HPP_ */
