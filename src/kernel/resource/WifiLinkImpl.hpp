/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_NETWORK_WIFI_HPP_
#define SIMGRID_KERNEL_NETWORK_WIFI_HPP_

#include "src/kernel/resource/models/network_cm02.hpp"
#include "xbt/ex.h"

/***********
 * Classes *
 ***********/

namespace simgrid::kernel::resource {

class XBT_PRIVATE WifiLinkAction;

class WifiLinkImpl : public StandardLinkImpl {
  /** @brief Hold every rates association between host and links (host name, rates id) */
  std::map<std::string, int, std::less<>> host_rates_;

  /** @brief A link can have several bandwidths attached to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

  bool use_callback_ = false;
  /*
   * Values used for the throughput degradation:
   * ratio = x0_ + co_acc_ * nb_active_flux_ / x0_
  **/
  /** @brief base maximum throughput to compare to when computing the ratio */
  const double x0_ = 5678270;
  /** @brief linear regression factor */
  const double co_acc_ = -5424;
  /** @brief minimum number of concurrent flows before using the linear regression */
  const int conc_lim_ = 20;
  /** @brief current concurrency on the link */
  int nb_active_flux_ = 0;

  std::vector<Metric> decay_bandwidths_;

public:
  WifiLinkImpl(const std::string& name, const std::vector<double>& bandwidths, lmm::System* system);

  void set_host_rate(const s4u::Host* host, int rate_level);
  /** @brief Get the AP rate associated to the host (or -1 if not associated to the AP) */
  double get_host_rate(const s4u::Host* host) const;

  s4u::Link::SharingPolicy get_sharing_policy() const override;
  void apply_event(kernel::profile::Event*, double) override { THROW_UNIMPLEMENTED; }
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  void set_latency(double) override;
  bool toggle_callback();

  static void update_bw_comm_end(const NetworkAction& action, Action::State state);
  void inc_active_flux();
  void dec_active_flux();
  static double wifi_link_dynamic_sharing(const WifiLinkImpl& link, double capacity, int n);
  double get_max_ratio() const;
  size_t get_host_count() const;
};

class WifiLinkAction : public NetworkCm02Action {
  WifiLinkImpl* src_wifi_link_;
  WifiLinkImpl* dst_wifi_link_;

public:
  WifiLinkAction() = delete;
  WifiLinkAction(Model* model, s4u::Host& src, s4u::Host& dst, double cost, bool failed, WifiLinkImpl* src_wifi_link,
                 WifiLinkImpl* dst_wifi_link)
      : NetworkCm02Action(model, src, dst, cost, failed), src_wifi_link_(src_wifi_link), dst_wifi_link_(dst_wifi_link)
  {
  }

  WifiLinkImpl* get_src_link() const { return src_wifi_link_; }
  WifiLinkImpl* get_dst_link() const { return dst_wifi_link_; }
};

} // namespace simgrid::kernel::resource
#endif
