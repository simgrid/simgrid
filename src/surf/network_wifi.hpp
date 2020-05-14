/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

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

  /** @brief A link can have several bandwith attach to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

  /** @brief Should we use the decay model ? */
  bool use_decay_model_=false;
  /** @brief Wifi ns-3 802.11n average bit rate */
  const double wifi_max_rate_=54*1e6 / 8;
  /** @brief ns-3 802.11n minimum bit rate */
  const double wifi_min_rate_=41.70837*1e6 / 8;
  /** @brief Decay model calibration */
  const int model_n_=5;
  /** @brief Decay model calibration: bitrate when using model_n_ stations */
  const double model_rate_=42.61438*1e6 / 8;
  /** @brief Decay model bandwidths */
  std::vector<Metric> decay_bandwidths_;

public:
  NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                  lmm::System* system);

  void set_host_rate(const s4u::Host* host, int rate_level);
  /** @brief Get the AP rate associated to the host (or -1 if not associated to the AP) */
  double get_host_rate(const s4u::Host* host);

  s4u::Link::SharingPolicy get_sharing_policy() override;
  void apply_event(kernel::profile::Event*, double) override { THROW_UNIMPLEMENTED; }
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  void set_latency(double) override { THROW_UNIMPLEMENTED; }
  void refresh_decay_bandwidths();
  bool toggle_decay_model();
};

class NetworkWifiAction : public NetworkCm02Action {
  NetworkWifiLink* src_wifi_link_;
  NetworkWifiLink* dst_wifi_link_;

public:
  NetworkWifiAction(Model* model, s4u::Host& src, s4u::Host& dst, double cost, bool failed,
                    NetworkWifiLink* src_wifi_link, NetworkWifiLink* dst_wifi_link)
      : NetworkCm02Action(model, src, dst, cost, failed)
      , src_wifi_link_(src_wifi_link)
      , dst_wifi_link_(dst_wifi_link){};

  NetworkWifiLink* get_src_link() const { return src_wifi_link_; }
  NetworkWifiLink* get_dst_link() const { return dst_wifi_link_; }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* SURF_NETWORK_WIFI_HPP_ */
