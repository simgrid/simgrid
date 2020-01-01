/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILE_DATEDVALUE
#define SIMGRID_KERNEL_PROFILE_DATEDVALUE

#include "simgrid/forward.h"
#include <iostream>

namespace simgrid {
namespace kernel {
namespace profile {

/** @brief Modeling of the availability profile (due to an external load) or the churn
 *
 * There is 4 main concepts in this module:
 * - #simgrid::kernel::profile::DatedValue: a pair <timestamp, value> (both are of type double)
 * - #simgrid::kernel::profile::Profile: a list of dated values
 * - #simgrid::kernel::profile::Event: links a given trace to a given SimGrid resource.
 *   A Cpu for example has 2 kinds of events: state (ie, is it ON/OFF) and speed,
 *   while a link has 3 iterators: state, bandwidth and latency.
 * - #simgrid::kernel::profile::FutureEvtSet: makes it easy to find the next occurring event of all profiles
 */
class XBT_PUBLIC DatedValue {
public:
  double date_          = 0;
  double value_         = 0;
  explicit DatedValue() = default;
  explicit DatedValue(double d, double v) : date_(d), value_(v) {}
  bool operator==(DatedValue const& e2) const;
  bool operator!=(DatedValue const& e2) const { return not(*this == e2); }
};
std::ostream& operator<<(std::ostream& out, const DatedValue& e);

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif
