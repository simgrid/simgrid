/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_REVERSIBLE_RACE_CALCULATOR_HPP
#define SIMGRID_MC_ODPOR_REVERSIBLE_RACE_CALCULATOR_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionActorJoin.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"

#include <memory>

namespace simgrid::mc::odpor {

/**
 * @brief Computes whether a race between two events
 * in a given execution is a reversible race.
 *
 * @note: All of the methods assume that there is
 * indeed a race between the two events in the
 * execution; indeed, the question the method answers
 * is only sensible in the context of a race
 */
struct ReversibleRaceCalculator final {
  static EventSet is_race_reversible(const Execution&, Execution::Handle e1, std::shared_ptr<Transition>);
  static EventSet is_race_reversible(const Execution&, Execution::Handle e1, std::shared_ptr<Transition>);
  static EventSet is_race_reversible(const Execution&, Execution::Handle e1, std::shared_ptr<Transition>);
  static EventSet is_race_reversible(const Execution&, Execution::Handle e1, std::shared_ptr<Transition>);

public:
  static EventSet is_race_reversible(const Execution&, Execution::Handle e1, Execution::Handle e2);
};

} // namespace simgrid::mc::odpor
#endif
