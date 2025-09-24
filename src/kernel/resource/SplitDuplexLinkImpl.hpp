/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP
#define SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP

#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

namespace simgrid::kernel::resource {

/************
 * Resource *
 ************/
/** @ingroup Model_network_interface
 * @brief Network link interface class
 * @details A Link represents the link between two [hosts](@ref HostImpl)
 */
class SplitDuplexLinkImpl : public LinkImpl {
  s4u::SplitDuplexLink piface_;
  StandardLinkImpl* link_up_;
  StandardLinkImpl* link_down_;

protected:
  SplitDuplexLinkImpl(const StandardLinkImpl&) = delete;
  SplitDuplexLinkImpl& operator=(const StandardLinkImpl&) = delete;

public:
  SplitDuplexLinkImpl(const std::string& name, StandardLinkImpl* link_up, StandardLinkImpl* link_down);
  /** @brief Public interface */
  const s4u::SplitDuplexLink* get_iface() const { return &piface_; }
  s4u::SplitDuplexLink* get_iface() { return &piface_; }

  /** @brief Get the bandwidth in bytes per second of current Link */
  double get_bandwidth() const override { return link_up_->get_bandwidth(); }
  void set_bandwidth(double value) override;

  /** @brief Get the latency in seconds of current Link */
  double get_latency() const override { return link_up_->get_latency(); }
  void set_latency(double value) override;

  /** @brief The sharing policy */
  void set_sharing_policy(s4u::Link::SharingPolicy policy, const s4u::NonLinearResourceCb& cb) override;

  /** @brief Get link composing this split-duplex link */
  s4u::Link* get_link_up() const { return link_up_->get_iface(); }
  s4u::Link* get_link_down() const { return link_down_->get_iface(); }

  /** @brief Check if the Link is used */
  bool is_used() const override;

  void turn_on() override;
  void turn_off() override;

  void seal() override;

  void apply_event(profile::Event* event, double value) override;

  /* setup the profile file with bandwidth events (peak speed changes due to external load).
   * Profile must contain percentages (value between 0 and 1). */
  void set_bandwidth_profile(kernel::profile::Profile* profile) override;
  /* setup the profile file with latency events (peak latency changes due to external load).
   * Profile must contain absolute values */
  void set_latency_profile(kernel::profile::Profile* profile) override;
  void set_concurrency_limit(int limit) const override;
  int get_concurrency_limit() const override;
};

} // namespace simgrid::kernel::resource

#endif /* SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP */
