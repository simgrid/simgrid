/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_LINKIMPLINTF_HPP
#define SIMGRID_KERNEL_RESOURCE_LINKIMPLINTF_HPP

#include "simgrid/s4u/Link.hpp"
#include "src/kernel/resource/Resource.hpp"
#include "xbt/PropertyHolder.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {
/************
 * Resource *
 ************/
/** @ingroup SURF_interface
 * @brief SURF network link interface class
 * @details A Link represents the link between two [hosts](@ref simgrid::surf::HostImpl)
 */
class LinkImplIntf : public Resource_T<LinkImplIntf>, public xbt::PropertyHolder {
public:
  using Resource_T::Resource_T;
  /** @brief Get the bandwidth in bytes per second of current Link */
  virtual double get_bandwidth() const = 0;
  /** @brief Update the bandwidth in bytes per second of current Link */
  virtual void set_bandwidth(double value) = 0;

  /** @brief Get the latency in seconds of current Link */
  virtual double get_latency() const = 0;
  /** @brief Update the latency in seconds of current Link */
  virtual void set_latency(double value) = 0;

  /** @brief The sharing policy */
  virtual void set_sharing_policy(s4u::Link::SharingPolicy policy, const s4u::NonLinearResourceCb& cb) = 0;
  virtual s4u::Link::SharingPolicy get_sharing_policy() const      = 0;

  /* setup the profile file with bandwidth events (peak speed changes due to external load).
   * Profile must contain percentages (value between 0 and 1). */
  virtual void set_bandwidth_profile(kernel::profile::Profile* profile) = 0;
  /* setup the profile file with latency events (peak latency changes due to external load).
   * Profile must contain absolute values */
  virtual void set_latency_profile(kernel::profile::Profile* profile) = 0;
  /** @brief Set the concurrency limit for this link */
  virtual void set_concurrency_limit(int limit) const = 0;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_RESOURCE_LINKIMPLINTF_HPP */
