/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_ACTOR_HPP
#define SIMGRID_MC_TRANSITION_ACTOR_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace simgrid::mc {

class ActorJoinTransition : public Transition {
  bool timeout_;
  Aid target_;

public:
  ActorJoinTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    // Joining is dependent with any transition whose
    // actor is that of the `other` action. , Join i
    if (other->aid_ == target_) {
      return true;
    }

    // Otherwise, joining is indep with any other transitions:
    // - It is only enabled once the target ends, and after this point it's enabled no matter what
    // - Other joins don't affect it, and it does not impact on the enabledness of any other transition
    return false;
  }
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  bool get_timeout() const { return timeout_; }
  /** Target ID */
  Aid get_target() const { return target_; }
};
class ActorExitTransition : public Transition {

public:
  ActorExitTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    // It is dependent with create and join, that are located before exit in the transition enum.
    // Otherwise, exiting is indep with any other transitions: it's equivalent to no-op
    return false;
  }
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class ActorSleepTransition : public Transition {

public:
  ActorSleepTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    // Sleeping is indep with any other transitions: always enabled, not impacted by any transition
    return false;
  }
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class ActorCreateTransition : public Transition {
  Aid child_;

public:
  ActorCreateTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    // Creation is dependent with any transition of the created actor (it's a local event to the created actor too)
    if (other->aid_ == child_)
      return true;

    // Creations are dependent with each other, as they compete for the given PID to each childs, so the interleavings
    // produce different results
    if (other->type_ == type_)
      return true;

    // Otherwise, creation is indep with any other transitions
    return false;
  }
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  /** ID of the created actor */
  Aid get_child() const { return child_; }
};

} // namespace simgrid::mc

#endif
