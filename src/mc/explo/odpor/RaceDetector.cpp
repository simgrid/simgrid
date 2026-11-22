/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/RaceDetector.hpp"

#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_race_detector, mc, "Data race detection of the model-checker");

namespace simgrid::mc::odpor {

void RaceDetector::check(const Execution& exec)
{
  // Find the prefix of the execution that did not change since the previous call. The execution only changes at its
  // end, and each transition belongs to a single state of the exploration tree, so the first match from the end gives
  // the common prefix.
  size_t common = std::min(known_.size(), exec.size());
  while (common > 0 && known_[common - 1].get() != exec.get_transition_for_handle(common - 1))
    common--;
  known_.resize(common);

  // An event whose accesses are in the shadow was removed: rebuild the shadow from scratch
  auto first = static_cast<Execution::EventHandle>(common);
  if (last_applied_.has_value() && *last_applied_ >= common) {
    XBT_DEBUG("Rebuild the shadow memory: event %u was removed", *last_applied_);
    shadow_.reset();
    last_applied_.reset();
    rebuild_count_++;
    first = 0;
  }

  for (Execution::EventHandle e = first; e < exec.size(); e++) {
    if (e >= known_.size())
      known_.emplace_back(exec.get_event_with_handle(e).get_transition());
    if (exec.get_transition_for_handle(e)->get_memory_trace_or_null() != nullptr) {
      last_applied_ = e; // Set before applying, so that the shadow gets rebuilt after a race
      apply(exec, e);
    }
  }
}

void RaceDetector::apply(const Execution& exec, Execution::EventHandle e)
{
  const Event& event = exec.get_event_with_handle(e);
  const Aid actor    = event.get_transition()->aid_;

  smemory::VectorClock thread_vc; // Every component is (u, 0) by default
  if (auto predecessor = event.get_predecessor(); predecessor.has_value()) {
    const ClockVector& cv = exec.get_event_with_handle(*predecessor).get_clock_vector();
    for (unsigned u = 0; u < static_config::max_threads - 1; u++)
      if (Clock c = cv.get(Aid(u)); c.has_value())
        thread_vc.clocks[u] = Epoch(Aid(u), Clock(c.value() + 1));
  }
  thread_vc.clocks[actor.value()] = Epoch(actor, Clock(e + 1));

  applied_count_++;
  shadow_.apply_transition(*event.get_transition()->get_memory_trace_or_null(), actor, thread_vc);
}

} // namespace simgrid::mc::odpor
