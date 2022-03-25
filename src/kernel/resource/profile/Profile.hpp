/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILE_HPP
#define SIMGRID_KERNEL_PROFILE_HPP

#include "simgrid/forward.h"
#include "simgrid/kernel/ProfileBuilder.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/StochasticDatedValue.hpp"

#include <queue>
#include <vector>
#include <string>

namespace simgrid {
namespace kernel {
namespace profile {

/** @brief A profile is a set of timed values, encoding the value that a variable takes at what time
 *
 * It is useful to model dynamic platforms, where an external load that makes the resource availability change over
 * time. To model that, you have to set several profiles per resource: one for the on/off state and one for each
 * numerical value (computational speed, bandwidth and/or latency).
 *
 * There are two behaviours. Either a callback is used to populate the profile when the set has been exhausted,
 * or the callback is called only during construction and the initial set is repeated over and over, after a fixed
 * repeating delay.
 */
class XBT_PUBLIC Profile {
public:
  /** @brief Create a profile. There are two behaviours. Either the callback is
   *
   * @param name The name of the profile (checked for uniqueness)
   * @param cb A callback object/function that populates the profile.
   * @param repeat_delay If strictly negative, it is ignored and the callback is called when an event reached the end of
   * the event_list. If zero or positive, the initial set repeats after the provided delay.
   */
  explicit Profile(const std::string& name, const std::function<ProfileBuilder::UpdateCb>& cb, double repeat_delay);
  virtual ~Profile()=default;
  Event* schedule(FutureEvtSet* fes, resource::Resource* resource);
  DatedValue next(Event* event);

  const std::vector<DatedValue>& get_event_list() const { return event_list; }
  const std::string& get_name() const { return name; }
  bool is_repeating() const { return repeat_delay>=0;}
  double get_repeat_delay() const { return repeat_delay;}

private:
  std::string name;
  std::function<ProfileBuilder::UpdateCb> cb;
  std::vector<DatedValue> event_list;
  FutureEvtSet* fes_  = nullptr;
  double repeat_delay;

  bool get_enough_events(size_t index)
  {
    if (index >= event_list.size() && cb)
      cb(event_list);
    return index < event_list.size();
  }
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

/** Module finalizer: frees all profiles */
XBT_PUBLIC void tmgr_finalize();

#endif
