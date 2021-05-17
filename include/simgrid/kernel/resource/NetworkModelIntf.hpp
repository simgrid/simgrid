/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP
#define SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP

#include <simgrid/forward.h>

#include <unordered_set>
#include <vector>

namespace simgrid {
namespace kernel {
namespace resource {

/** @ingroup SURF_interface
 * @brief Network Model interface class
 */
class XBT_PUBLIC NetworkModelIntf {
public:
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
  using NetworkFactorCb = double(double size, const s4u::Host* src, const s4u::Host* dst,
                                 const std::vector<s4u::Link*>& links,
                                 const std::unordered_set<s4u::NetZone*>& netzones);
  /** @brief Configure the latency factor callback */
  virtual void set_lat_factor_cb(const std::function<NetworkFactorCb>& cb) = 0;
  /** @brief Configure the bandwidth factor callback */
  virtual void set_bw_factor_cb(const std::function<NetworkFactorCb>& cb) = 0;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP */