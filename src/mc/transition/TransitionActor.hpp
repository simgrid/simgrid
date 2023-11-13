/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

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
  aid_t target_;

public:
  ActorJoinTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  bool get_timeout() const { return timeout_; }
  /** Target ID */
  aid_t get_target() const { return target_; }
};

class ActorSleepTransition : public Transition {

public:
  ActorSleepTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;
};

} // namespace simgrid::mc

#endif
