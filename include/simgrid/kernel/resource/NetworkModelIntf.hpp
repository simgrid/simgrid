/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP
#define SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP

#include <simgrid/s4u/Link.hpp>
#include <simgrid/s4u/NetZone.hpp>

#include <unordered_set>
#include <vector>

namespace simgrid {
namespace kernel {
namespace resource {

/** @ingroup SURF_interface
 * @brief Network Model interface class
 * @details Defines the methods that a Network model must implement
 */
class XBT_PUBLIC NetworkModelIntf {
public:
  using NetworkFactorCb     = double(double size, const std::vector<s4u::Link*>& links,
                                 const std::unordered_set<s4u::NetZone*>& netzones);
  virtual void set_lat_factor_cb(const std::function<NetworkFactorCb>& cb)        = 0;
  virtual void set_bw_factor_cb(const std::function<NetworkFactorCb>& cb)         = 0;
};

} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_RESOURCE_NETWORKMODELINTF_HPP */