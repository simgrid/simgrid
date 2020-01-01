/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_wifi.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

namespace simgrid {
namespace kernel {
namespace resource {

/************
 * Resource *
 ************/

NetworkWifiLink::NetworkWifiLink(NetworkCm02Model* model, const std::string& name, std::vector<double> bandwidths,
                                 lmm::System* system)
    : LinkImpl(model, name, system->constraint_new(this, 1))
{
  for (auto bandwidth : bandwidths)
    bandwidths_.push_back({bandwidth, 1.0, nullptr});

  simgrid::s4u::Link::on_creation(*get_iface());
}

void NetworkWifiLink::set_host_rate(const s4u::Host* host, int rate_level)
{
  auto insert_done = host_rates_.insert(std::make_pair(host->get_name(), rate_level));
  if (insert_done.second == false)
    insert_done.first->second = rate_level;
}

double NetworkWifiLink::get_host_rate(const s4u::Host* host)
{
  std::map<xbt::string, int>::iterator host_rates_it;
  host_rates_it = host_rates_.find(host->get_name());

  if (host_rates_it == host_rates_.end())
    return -1;

  int rate_id = host_rates_it->second;
  xbt_assert(rate_id >= 0 && rate_id < (int)bandwidths_.size(), "Host '%s' has an invalid rate '%d' on wifi link '%s'",
             host->get_name().c_str(), rate_id, this->get_cname());

  Metric rate = bandwidths_[rate_id];
  return rate.peak * rate.scale;
}

s4u::Link::SharingPolicy NetworkWifiLink::get_sharing_policy()
{
  return s4u::Link::SharingPolicy::WIFI;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
