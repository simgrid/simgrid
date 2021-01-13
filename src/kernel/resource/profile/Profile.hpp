/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILE_HPP
#define SIMGRID_KERNEL_PROFILE_HPP

#include "simgrid/forward.h"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/StochasticDatedValue.hpp"

#include <queue>
#include <vector>

namespace simgrid {
namespace kernel {
namespace profile {

/** @brief A profile is a set of timed values, encoding the value that a variable takes at what time
 *
 * It is useful to model dynamic platforms, where an external load that makes the resource availability change over
 * time. To model that, you have to set several profiles per resource: one for the on/off state and one for each
 * numerical value (computational speed, bandwidth and/or latency).
 */
class XBT_PUBLIC Profile {
public:
  /**  Creates an empty trace */
  explicit Profile();
  virtual ~Profile();
  Event* schedule(FutureEvtSet* fes, resource::Resource* resource);
  DatedValue next(Event* event);

  static Profile* from_file(const std::string& path);
  static Profile* from_string(const std::string& name, const std::string& input, double periodicity);
  // private:
  std::vector<DatedValue> event_list;
  std::vector<StochasticDatedValue> stochastic_event_list;

private:
  FutureEvtSet* fes_  = nullptr;
  bool stochastic     = false;
  bool stochasticloop = false;
  DatedValue futureDV;
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

/** Module finalizer: frees all profiles */
XBT_PUBLIC void tmgr_finalize();

#endif
