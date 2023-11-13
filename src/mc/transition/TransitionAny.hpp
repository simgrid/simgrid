/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_ANY_HPP
#define SIMGRID_MC_TRANSITION_ANY_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionComm.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace simgrid::mc {

class TestAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  Transition* get_current_transition() const { return transitions_.at(times_considered_); }
  bool result() const
  {
    return std::any_of(begin(transitions_), end(transitions_), [](const Transition* transition) {
      const auto* tested_transition = static_cast<const CommTestTransition*>(transition);
      return (tested_transition->get_sender() != -1 && tested_transition->get_receiver() != -1);
    });
  }
};

class WaitAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;

  Transition* get_current_transition() const { return transitions_.at(times_considered_); }
};

} // namespace simgrid::mc

#endif
