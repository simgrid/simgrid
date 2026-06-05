/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_ANY_HPP
#define SIMGRID_MC_TRANSITION_ANY_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionComm.hpp"

#include <algorithm>
#include <string>

namespace simgrid::mc {

class TestAnyTransition final : public Transition {
  std::vector<Transition*> transitions_;

public:
  TestAnyTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    return this->get_current_transition()->dispatch_depends(other);
  }

  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  Transition* get_current_transition() const { return transitions_.at(times_considered_); }
  bool result() const
  {
    return std::any_of(begin(transitions_), end(transitions_), [](const Transition* transition) {
      const auto* tested_transition = static_cast<const CommTestTransition*>(transition);
      return (tested_transition->get_sender().has_value() && tested_transition->get_receiver().has_value());
    });
  }
};

class WaitAnyTransition final : public Transition {
  std::vector<Transition*> transitions_;

public:
  WaitAnyTransition(Aid issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    return this->get_current_transition()->dispatch_depends(other);
  }

  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;

  Transition* get_current_transition() const;
};

} // namespace simgrid::mc

#endif
