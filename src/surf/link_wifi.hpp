/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_LINK_WIFI_HPP_
#define SURF_LINK_WIFI_HPP_

#include "network_cm02.hpp"
#include "network_interface.hpp"
#include "src/surf/HostImpl.hpp"
#include "xbt/string.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

class NetworkWifiLink : public NetworkCm02Link {
  /** @brief Hold every rates association between host and links (host name, rates id) */
  std::map<xbt::string, int> host_rates_;

  /** @brief Hold every rates available for this Access Point */
  // double* rates; FIXME: unused

  /** @brief A link can have several bandwith attach to it (mostly use by wifi model) */
  std::vector<Metric> bandwidths_;

public:
  NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                  s4u::Link::SharingPolicy policy, lmm::System* system);

  void set_host_rate(sg_host_t host, int rate_level);
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif
