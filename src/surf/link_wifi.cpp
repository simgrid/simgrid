/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/link_wifi.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

NetworkWifiLink::NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                                 s4u::Link::SharingPolicy policy, lmm::System* system)
    : NetworkCm02Link(model, name, 0, 0, policy, system)
{
  for (auto bandwith : bandwidths) {
    bandwidths_.push_back({bandwith, 1.0, nullptr});
  }
}

void NetworkWifiLink::set_host_rate(sg_host_t host, int rate_level)
{
  host_rates_.insert(std::make_pair(host->get_name(), rate_level));
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
