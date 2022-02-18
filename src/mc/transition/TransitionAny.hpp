/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_ANY_HPP
#define SIMGRID_MC_TRANSITION_ANY_HPP

#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"

#include <sstream>
#include <string>

namespace simgrid {
namespace mc {

class TestAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  TestAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  Transition* get_current_transition() const { return transitions_.at(times_considered_); }
};

class WaitAnyTransition : public Transition {
  std::vector<Transition*> transitions_;

public:
  WaitAnyTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;

  Transition* get_current_transition() const { return transitions_.at(times_considered_); }
};

} // namespace mc
} // namespace simgrid

#endif
