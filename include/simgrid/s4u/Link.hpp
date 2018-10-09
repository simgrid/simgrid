/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_LINK_HPP_
#define S4U_LINK_HPP_

#include <simgrid/kernel/resource/Action.hpp>
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
/** @brief A Link represents the network facilities between [hosts](@ref simgrid::s4u::Host) */
class XBT_PUBLIC Link : public simgrid::xbt::Extendable<Link> {
  friend simgrid::kernel::resource::LinkImpl;

  // Links are created from the NetZone, and destroyed by their private implementation when the simulation ends
  explicit Link(kernel::resource::LinkImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Link() = default;
  // The private implementation, that never changes
  kernel::resource::LinkImpl* const pimpl_;

public:
  enum class SharingPolicy { SPLITDUPLEX = 2, SHARED = 1, FATPIPE = 0 };

  kernel::resource::LinkImpl* get_impl() { return pimpl_; }

  /** @brief Retrieve a link from its name */
  static Link* by_name(std::string name);
  static Link* by_name_or_null(std::string name);

  /** @brief Retrieves the name of that link as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that link as a C string */
  const char* get_cname() const;

  /** @brief Get the bandwidth in bytes per second of current Link */
  double get_bandwidth();

  /** @brief Get the latency in seconds of current Link */
  double get_latency();

  /** @brief Describes how the link is shared between flows */
  SharingPolicy get_sharing_policy();

  /** @brief Returns the current load (in flops per second) */
  double get_usage();

  /** @brief Check if the Link is used (at least one flow uses the link) */
  bool is_used();

  void turn_on();
  void turn_off();

  void* get_data(); /** Should be used only from the C interface. Prefer extensions in C++ */
  void set_data(void* d);

  void set_state_trace(tmgr_trace_t trace);     /*< setup the trace file with states events (ON or OFF). Trace must contain
                                                  boolean values. */
  void set_bandwidth_trace(tmgr_trace_t trace); /*< setup the trace file with bandwidth events (peak speed changes due to
                                                  external load). Trace must contain percentages (value between 0 and 1). */
  void set_latency_trace(tmgr_trace_t trace);   /*< setup the trace file with latency events (peak latency changes due to
                                                  external load). Trace must contain absolute values */

  const char* get_property(std::string key);
  void set_property(std::string key, std::string value);

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
  static simgrid::xbt::signal<void(kernel::resource::NetworkAction*, kernel::resource::Action::State)>
      on_communication_state_change;

  // Deprecated methods
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::by_name()") static Link* byName(const char* name) { return by_name(name); }
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_name()") const std::string& getName() const { return get_name(); }
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_cname()") const char* getCname() const { return get_cname(); }
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_sharing_policy()") SharingPolicy sharingPolicy() {return get_sharing_policy();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_usage()") double getUsage() {return get_usage();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::is_used()") bool isUsed() {return is_used();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_bandwidth()") double bandwidth() {return get_bandwidth();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_latency()") double latency() {return get_latency();}

  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::turn_on()") void turnOn() {turn_on();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::turn_off()") void turnOff() {turn_off();}

  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_property()") const char* getProperty(const char* key) {return get_property(key);}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::set_property()") void setProperty(std::string key, std::string value) {set_property(key, value);}

  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_data()") void* getData() {return get_data();}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::set_data()") void setData(void* d) {set_data(d);}

  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_state_trace()") void setStateTrace(tmgr_trace_t trace) {set_state_trace(trace);}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_bandwidth_trace()") void setBandwidthTrace(tmgr_trace_t trace) {set_bandwidth_trace(trace);}
  /** @deprecated */
  XBT_ATTRIB_DEPRECATED_v323("Please use Link::get_latency_trace()") void setLatencyTrace(tmgr_trace_t trace) {set_latency_trace(trace);}
};
}
}

#endif /* SURF_NETWORK_INTERFACE_HPP_ */
