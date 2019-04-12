/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef PROFILE_HPP
#define PROFILE_HPP

#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include <queue>
#include <vector>

namespace simgrid {
namespace kernel {
namespace profile {

class Event {
public:
  Profile* profile;
  unsigned int idx;
  resource::Resource* resource;
  bool free_me;
};

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

private:
  FutureEvtSet* fes_ = nullptr;
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif