/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SURF_NETWORK_SMPI_HPP
#define SIMGRID_SURF_NETWORK_SMPI_HPP

#include <xbt/base.h>

#include "network_cm02.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE NetworkSmpiModel : public NetworkCm02Model {
public:
  explicit NetworkSmpiModel(std::string name);

  double get_latency_factor(double size) override;
  double get_bandwidth_factor(double size) override;
  double get_bandwidth_constraint(double rate, double bound, double size) override;
};
} // namespace resource
} // namespace kernel
}

#endif
