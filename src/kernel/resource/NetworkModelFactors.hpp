/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_NETWORKMODELFACTORS_HPP
#define SIMGRID_KERNEL_RESOURCE_NETWORKMODELFACTORS_HPP

#include "src/kernel/resource/FactorSet.hpp"
#include "src/simgrid/sg_config.hpp"
#include "xbt/asserts.h"
#include <simgrid/forward.h>

#include <unordered_set>
#include <vector>

namespace simgrid::kernel::resource {

/** This Trait of NetworkModel is in charge of handling the network factors (bw and lat) */

class XBT_PUBLIC NetworkModelFactors {
  static FactorSet cfg_latency_factor;
  static FactorSet cfg_bandwidth_factor;

  using NetworkFactorCb = double(double size, const s4u::Host* src, const s4u::Host* dst,
                                 const std::vector<s4u::Link*>& links,
                                 const std::unordered_set<s4u::NetZone*>& netzones);

  std::function<NetworkFactorCb> lat_factor_cb_;
  std::function<NetworkFactorCb> bw_factor_cb_;

public:
  /**
   * @brief Get the right multiplicative factor for the latency.
   * @details Depending on the model, the effective latency when sending a message might be different from the
   * theoretical latency of the link, in function of the message size. In order to account for this, this function gets
   * this factor.
   */
  double get_latency_factor(double size, const s4u::Host* src, const s4u::Host* dst,
                            const std::vector<s4u::Link*>& links,
                            const std::unordered_set<s4u::NetZone*>& netzones) const;

  /** Get the right multiplicative factor for the bandwidth (only if no callback was defined) */
  double get_latency_factor() const;

  /**
   * @brief Get the right multiplicative factor for the bandwidth.
   *
   * @details Depending on the model, the effective bandwidth when sending a message might be different from the
   * theoretical bandwidth of the link, in function of the message size. In order to account for this, this function
   * gets this factor.
   */
  double get_bandwidth_factor(double size, const s4u::Host* src, const s4u::Host* dst,
                              const std::vector<s4u::Link*>& links,
                              const std::unordered_set<s4u::NetZone*>& netzones) const;

  /** Get the right multiplicative factor for the bandwidth (only if no callback was defined) */
  double get_bandwidth_factor() const;

  /**
   * @brief Callback to set the bandwidth and latency factors used in a communication
   *
   * This callback offers more flexibility when setting the network factors.
   * It is an alternative to SimGrid's configs, such as network/latency-factors
   * and network/bandwidth-factors.
   *
   * @param size Communication size in bytes
   * @param src Source host
   * @param dst Destination host
   * @param links Vectors with the links used in this comm
   * @param netzones Set with NetZones involved in the comm
   * @return Multiply factor
   */
  /** @brief Configure the latency factor callback */
  void set_lat_factor_cb(const std::function<NetworkFactorCb>& cb);

  /** @brief Configure the bandwidth factor callback */
  void set_bw_factor_cb(const std::function<NetworkFactorCb>& cb);

  /** Returns whether a callback was set for latency-factor OR bandwidth-factor */
  bool has_network_factor_cb() const { return lat_factor_cb_ || bw_factor_cb_; }
};

} // namespace simgrid::kernel::resource

#endif /* SIMGRID_KERNEL_RESOURCE_NETWORKMODELFACTORS_HPP */
