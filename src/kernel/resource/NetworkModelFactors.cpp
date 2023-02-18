/* Copyright (c) 2013-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/NetworkModelFactors.hpp"
#include "src/simgrid/sg_config.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/

namespace simgrid::kernel::resource {
config::Flag<std::string> cfg_latency_factor_str(
    "network/latency-factor", std::initializer_list<const char*>{"smpi/lat-factor"},
    "Correction factor to apply to the provided latency (default value overridden by network model)", "1.0");
static config::Flag<std::string> cfg_bandwidth_factor_str(
    "network/bandwidth-factor", std::initializer_list<const char*>{"smpi/bw-factor"},
    "Correction factor to apply to the provided bandwidth (default value overridden by network model)", "1.0");

FactorSet NetworkModelFactors::cfg_latency_factor("network/latency-factor");
FactorSet NetworkModelFactors::cfg_bandwidth_factor("network/bandwidth-factor");

double NetworkModelFactors::get_bandwidth_factor() const
{
  xbt_assert(not bw_factor_cb_,
             "Cannot access the global bandwidth factor since a callback is used. Please go for the advanced API.");

  if (not cfg_bandwidth_factor.is_initialized())
    cfg_bandwidth_factor.parse(cfg_bandwidth_factor_str.get());

  return cfg_bandwidth_factor(0);
}

double NetworkModelFactors::get_latency_factor() const
{
  xbt_assert(not lat_factor_cb_,
             "Cannot access the global latency factor since a callback is used. Please go for the advanced API.");

  if (not cfg_latency_factor.is_initialized()) // lazy initiaization to avoid initialization fiasco
    cfg_latency_factor.parse(cfg_latency_factor_str.get());

  return cfg_latency_factor(0);
}

double NetworkModelFactors::get_latency_factor(double size, const s4u::Host* src, const s4u::Host* dst,
                                               const std::vector<s4u::Link*>& links,
                                               const std::unordered_set<s4u::NetZone*>& netzones) const
{
  if (lat_factor_cb_)
    return lat_factor_cb_(size, src, dst, links, netzones);

  if (not cfg_latency_factor.is_initialized()) // lazy initiaization to avoid initialization fiasco
    cfg_latency_factor.parse(cfg_latency_factor_str.get());

  return cfg_latency_factor(size);
}

double NetworkModelFactors::get_bandwidth_factor(double size, const s4u::Host* src, const s4u::Host* dst,
                                                 const std::vector<s4u::Link*>& links,
                                                 const std::unordered_set<s4u::NetZone*>& netzones) const
{
  if (bw_factor_cb_)
    return bw_factor_cb_(size, src, dst, links, netzones);

  if (not cfg_bandwidth_factor.is_initialized())
    cfg_bandwidth_factor.parse(cfg_bandwidth_factor_str.get());

  return cfg_bandwidth_factor(size);
}

void NetworkModelFactors::set_lat_factor_cb(const std::function<NetworkFactorCb>& cb)
{
  if (not cb)
    throw std::invalid_argument("NetworkModelFactors: Invalid callback");
  if (not simgrid::config::is_default("network/latency-factor"))
    throw std::invalid_argument("You must choose between network/latency-factor and callback configuration.");

  lat_factor_cb_ = cb;
}

void NetworkModelFactors::set_bw_factor_cb(const std::function<NetworkFactorCb>& cb)
{
  if (not cb)
    throw std::invalid_argument("NetworkModelFactors: Invalid callback");
  if (not simgrid::config::is_default("network/bandwidth-factor"))
    throw std::invalid_argument("You must choose between network/bandwidth-factor and callback configuration.");

  bw_factor_cb_ = cb;
}

} // namespace simgrid::kernel::resource
