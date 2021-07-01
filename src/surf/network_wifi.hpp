/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_NETWORK_WIFI_HPP_
#define SURF_NETWORK_WIFI_HPP_

#include <xbt/base.h>

#include "network_cm02.hpp"
#include "xbt/string.hpp"

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE NetworkWifiAction;

class NetworkWifiLink : public LinkImpl {
  /** @brief Hold every rates association between host and links (host name, rates id) */
  std::map<xbt::string, int> host_rates_;

  /** @brief A link can have several bandwidths attached to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

  /** @brief Should we use the decay model ? */
  bool use_decay_model_=false;
  /** @brief Wifi maximal bit rate according to the ns-3 802.11n standard */
  const double wifi_max_rate_ = 54 * 1e6 / 8;
  /** @brief minimum bit rate observed with ns3 during our calibration experiments */
  const double wifi_min_rate_ = 41.70837 * 1e6 / 8;
  /** @brief Amount of stations used in the reference point to rescale SimGrid predictions to fit ns-3 ones */
  const int model_n_=5;
  /** @brief Bit rate observed on ns3 at the reference point used for rescaling */
  const double model_rate_=42.61438*1e6 / 8;
  /** @brief The bandwidth to use for each SNR level, corrected with the decay rescale mechanism */
  std::vector<Metric> decay_bandwidths_;

public:
  NetworkWifiLink(const std::string& name, const std::vector<double>& bandwidths, lmm::System* system);

  void set_host_rate(const s4u::Host* host, int rate_level);
  /** @brief Get the AP rate associated to the host (or -1 if not associated to the AP) */
  double get_host_rate(const s4u::Host* host) const;

  s4u::Link::SharingPolicy get_sharing_policy() const override;
  void apply_event(kernel::profile::Event*, double) override { THROW_UNIMPLEMENTED; }
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  void set_latency(double) override;
  void refresh_decay_bandwidths();
  bool toggle_decay_model();
  int get_host_count() const;
};

class NetworkWifiAction : public NetworkCm02Action {
  NetworkWifiLink* src_wifi_link_;
  NetworkWifiLink* dst_wifi_link_;

public:
  NetworkWifiAction() = delete;
  NetworkWifiAction(Model* model, s4u::Host& src, s4u::Host& dst, double cost, bool failed,
                    NetworkWifiLink* src_wifi_link, NetworkWifiLink* dst_wifi_link)
      : NetworkCm02Action(model, src, dst, cost, failed)
      , src_wifi_link_(src_wifi_link)
      , dst_wifi_link_(dst_wifi_link)
    {}

  NetworkWifiLink* get_src_link() const { return src_wifi_link_; }
  NetworkWifiLink* get_dst_link() const { return dst_wifi_link_; }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* SURF_NETWORK_WIFI_HPP_ */
