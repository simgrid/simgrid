/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILEBUILDER_HPP
#define SIMGRID_KERNEL_PROFILEBUILDER_HPP

#include <simgrid/forward.h>
#include <functional>

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

std::ostream& operator << (std::ostream& out, const DatedValue& dv);
/**
 * @brief Simple builder for Profile classes.
 *
 * It can be used to create profiles for links, hosts or disks.
 */
class XBT_PUBLIC ProfileBuilder {
public:
  /** @brief A function called to populate the set of timed-values of a profile.
   *
   * When the profile is repeating, the callback is called only once upon Profile construction.
   * Then the timed values are repeated after a fixed repeat_delay.
   *
   * When the profile is not repeating, the callback is called each time the profile data has been exhausted.
   * More precisely, each time an FutureEvtSet reached the end of the vector of timed values of the profile.
   *
   * Note that the callback is only supposed to append values to the values vector, not modify or remove existing ones,
   * because other FutureEvtSet may still need previous values.
   *
   * @param values The vector of the profile values, where new data can be appended.
   *
   */
  using UpdateCb = void(std::vector<DatedValue>& values);

  static Profile* from_file(const std::string& path);
  static Profile* from_string(const std::string& name, const std::string& input, double periodicity);

  static Profile* from_void();

  /** Create a profile from a callback
   *
   * @param name: The *unique* name of the profile
   * @param cb: A callback object/function to populate the profile
   * @param repeat_delay: Ignored if strictly negative. Otherwise, profile is repeating and repeat_delay is inserted
   * between two iterations.
   *
   * @return the newly created profile
   */
  static Profile* from_callback(const std::string& name, const std::function<UpdateCb>& cb, double repeat_delay);
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_PROFILEBUILDER_HPP */
