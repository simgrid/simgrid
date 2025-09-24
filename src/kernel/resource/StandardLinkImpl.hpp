/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_STANDARDLINKIMPL_HPP
#define SIMGRID_KERNEL_RESOURCE_STANDARDLINKIMPL_HPP

#include "src/kernel/resource/LinkImpl.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid::kernel::resource {
/************
 * Resource *
 ************/
class StandardLinkImpl : public LinkImpl {
  s4u::Link piface_;

protected:
  explicit StandardLinkImpl(const std::string& name, s4u::Link::SharingPolicy sharing_policy,
                            routing::NetZoneImpl* englobing_zone);
  StandardLinkImpl(const StandardLinkImpl&) = delete;
  StandardLinkImpl& operator=(const StandardLinkImpl&) = delete;
  ~StandardLinkImpl() override                         = default; // Use destroy() instead of this destructor.

  Metric latency_   = {0.0, 1, nullptr};
  Metric bandwidth_ = {1.0, 1, nullptr};

public:
  void destroy(); // Must be called instead of the destructor
  class Deleter {
  public:
    void operator()(StandardLinkImpl* link) const;
  };

  void latency_check(double latency) const;

  /** @brief Public interface */
  const s4u::Link* get_iface() const { return &piface_; }
  s4u::Link* get_iface() { return &piface_; }

  /** @brief Get the bandwidth in bytes per second of current Link */
  double get_bandwidth() const override { return bandwidth_.peak * bandwidth_.scale; }

  /** @brief Get the latency in seconds of current Link */
  double get_latency() const override { return latency_.peak * latency_.scale; }

  /** @brief The sharing policy */
  void set_sharing_policy(s4u::Link::SharingPolicy policy, const s4u::NonLinearResourceCb& cb) override;

  void turn_on() override;
  void turn_off() override;

  void seal() override;

  void on_bandwidth_change() const;

  /* setup the profile file with bandwidth events (peak speed changes due to external load).
   * Profile must contain percentages (value between 0 and 1). */
  void set_bandwidth_profile(kernel::profile::Profile* profile) override;
  /* setup the profile file with latency events (peak latency changes due to external load).
   * Profile must contain absolute values */
  void set_latency_profile(kernel::profile::Profile* profile) override;
};

} // namespace simgrid::kernel::resource

#endif /* SIMGRID_KERNEL_RESOURCE_STANDARDLINKIMPL_HPP */
