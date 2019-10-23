/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

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

class NetworkWifiLink : public LinkImpl {
  /** @brief Hold every rates association between host and links (host name, rates id) */
  std::map<xbt::string, int> host_rates_;

  /** @brief A link can have several bandwith attach to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

public:
  NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                  lmm::System* system);

  void set_host_rate(s4u::Host* host, int rate_level);
  /** @brief Get the AP rate associated to the host (or -1 if not associated to the AP) */
  double get_host_rate(s4u::Host* host);

  s4u::Link::SharingPolicy get_sharing_policy() override;
  void apply_event(kernel::profile::Event*, double) override { THROW_UNIMPLEMENTED; }
  void set_bandwidth(double) override { THROW_UNIMPLEMENTED; }
  void set_latency(double) override { THROW_UNIMPLEMENTED; }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* SURF_NETWORK_WIFI_HPP_ */
