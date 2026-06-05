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

class ActorJoinTransition final : public Transition {
  bool timeout_;
  Aid target_;

public:
  ActorJoinTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  bool get_timeout() const { return timeout_; }
  /** Target ID */
  Aid get_target() const { return target_; }
};

class ActorExitTransition final : public Transition {

public:
  ActorExitTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class ActorSleepTransition final : public Transition {

public:
  ActorSleepTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

class ActorCreateTransition final : public Transition {
  Aid child_;

public:
  ActorCreateTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool can_be_co_enabled(const Transition* other) const override;
  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  /** ID of the created actor */
  Aid get_child() const { return child_; }
};

} // namespace simgrid::mc

#endif
