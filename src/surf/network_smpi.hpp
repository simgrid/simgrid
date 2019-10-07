/* Copyright (c) 2013-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "network_cm02.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

class XBT_PRIVATE NetworkSmpiModel : public NetworkCm02Model {
public:
  NetworkSmpiModel();
  ~NetworkSmpiModel() = default;

  double get_latency_factor(double size);
  double get_bandwidth_factor(double size);
  double get_bandwidth_constraint(double rate, double bound, double size);
};
} // namespace resource
} // namespace kernel
}
