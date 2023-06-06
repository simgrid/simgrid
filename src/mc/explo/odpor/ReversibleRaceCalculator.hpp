/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_REVERSIBLE_RACE_CALCULATOR_HPP
#define SIMGRID_MC_ODPOR_REVERSIBLE_RACE_CALCULATOR_HPP

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
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
class ReversibleRaceCalculator final {
  static bool is_race_reversible_ActorJoin(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_BarrierAsyncLock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_BarrierWait(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_CommRecv(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_CommSend(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_CommWait(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_CommTest(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_MutexAsyncLock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_MutexTest(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_MutexTrylock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_MutexUnlock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_MutexWait(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_SemAsyncLock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_SemUnlock(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_SemWait(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_ObjectAccess(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_Random(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_TestAny(const Execution&, Execution::EventHandle e1, const Transition* e2);
  static bool is_race_reversible_WaitAny(const Execution&, Execution::EventHandle e1, const Transition* e2);

public:
  static bool is_race_reversible(const Execution&, Execution::EventHandle e1, Execution::EventHandle e2);
};

} // namespace simgrid::mc::odpor
#endif
