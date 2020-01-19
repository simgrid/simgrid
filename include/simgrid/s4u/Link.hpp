/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef S4U_LINK_HPP
#define S4U_LINK_HPP

#include <simgrid/forward.h>
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

extern template class XBT_PUBLIC xbt::Extendable<s4u::Link>;

namespace s4u {
/** @brief A Link represents the network facilities between [hosts](@ref simgrid::s4u::Host) */
class XBT_PUBLIC Link : public xbt::Extendable<Link> {
  friend kernel::resource::LinkImpl;

  // Links are created from the NetZone, and destroyed by their private implementation when the simulation ends
  explicit Link(kernel::resource::LinkImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Link() = default;
  // The private implementation, that never changes
  kernel::resource::LinkImpl* const pimpl_;

public:
  enum class SharingPolicy { WIFI = 3, SPLITDUPLEX = 2, SHARED = 1, FATPIPE = 0 };

  kernel::resource::LinkImpl* get_impl() const { return pimpl_; }

  /** @brief Retrieve a link from its name */
  static Link* by_name(const std::string& name);
  static Link* by_name_or_null(const std::string& name);

  /** @brief Retrieves the name of that link as a C++ string */
  const std::string& get_name() const;
  /** @brief Retrieves the name of that link as a C string */
  const char* get_cname() const;

  /** @brief Get the bandwidth in bytes per second of current Link */
  double get_bandwidth() const;

  /** @brief Get the latency in seconds of current Link */
  double get_latency() const;

  /** @brief Describes how the link is shared between flows */
  SharingPolicy get_sharing_policy() const;

  /** @brief Returns the current load (in flops per second) */
  double get_usage() const;

  /** @brief Check if the Link is used (at least one flow uses the link) */
  bool is_used() const;

  void turn_on();
  bool is_on() const;
  void turn_off();

  /** Setup the profile with states events (ON or OFF). The profile must contain boolean values. */
  void set_state_profile(kernel::profile::Profile* profile);
  /** Setup the profile with bandwidth events (peak speed changes due to external load).
   * The profile must contain percentages (value between 0 and 1). */
  void set_bandwidth_profile(kernel::profile::Profile* profile);
  /** Setup the profile file with latency events (peak latency changes due to external load).
   * The profile must contain absolute values */
  void set_latency_profile(kernel::profile::Profile* profile);

  const char* get_property(const std::string& key) const;
  void set_property(const std::string& key, const std::string& value);

  /* The signals */
  /** @brief Callback signal fired when a new Link is created */
  static xbt::signal<void(Link&)> on_creation;

  /** @brief Callback signal fired when a Link is destroyed */
  static xbt::signal<void(Link const&)> on_destruction;

  /** @brief Callback signal fired when the state of a Link changes (when it is turned on or off) */
  static xbt::signal<void(Link const&)> on_state_change;

  /** @brief Callback signal fired when the bandwidth of a Link changes */
  static xbt::signal<void(Link const&)> on_bandwidth_change;

  /** @brief Callback signal fired when a communication starts */
  static xbt::signal<void(kernel::resource::NetworkAction&, Host* src, Host* dst)> on_communicate;

  /** @brief Callback signal fired when a communication changes it state (ready/done/cancel) */
  static xbt::signal<void(kernel::resource::NetworkAction&, kernel::resource::Action::State)>
      on_communication_state_change;
};
} // namespace s4u
} // namespace simgrid

#endif /* S4U_LINK_HPP */
