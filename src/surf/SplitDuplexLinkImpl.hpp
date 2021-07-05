/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP
#define SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP

#include "src/surf/LinkImpl.hpp"
#include "src/surf/LinkImplIntf.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {
/************
 * Resource *
 ************/
/** @ingroup SURF_network_interface
 * @brief SURF network link interface class
 * @details A Link represents the link between two [hosts](@ref simgrid::surf::HostImpl)
 */
class SplitDuplexLinkImpl : public LinkImplIntf {
  s4u::SplitDuplexLink piface_;
  s4u::Link::SharingPolicy sharing_policy_ = s4u::Link::SharingPolicy::SPLITDUPLEX;
  LinkImpl* link_up_;
  LinkImpl* link_down_;

protected:
  SplitDuplexLinkImpl(const LinkImpl&) = delete;
  SplitDuplexLinkImpl& operator=(const LinkImpl&) = delete;

public:
  SplitDuplexLinkImpl(const std::string& name, LinkImpl* link_up, LinkImpl* link_down);
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
  void set_sharing_policy(s4u::Link::SharingPolicy policy) override;
  s4u::Link::SharingPolicy get_sharing_policy() const override;

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
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_RESOURCE_SDLINKIMPL_HPP */
