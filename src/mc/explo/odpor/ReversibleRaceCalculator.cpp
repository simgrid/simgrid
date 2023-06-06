/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/ReversibleRaceCalculator.hpp"
#include "src/mc/explo/odpor/Execution.hpp"

#include <functional>
#include <unordered_map>
#include <xbt/asserts.h>
#include <xbt/ex.h>

namespace simgrid::mc::odpor {

bool ReversibleRaceCalculator::is_race_reversible(const Execution& E, Execution::EventHandle e1,
                                                  Execution::EventHandle e2)
{
  using Action     = Transition::Type;
  using Handler    = std::function<bool(const Execution&, Execution::EventHandle, const Transition*)>;
  using HandlerMap = std::unordered_map<Action, Handler>;

  const static HandlerMap handlers =
      HandlerMap{{Action::ACTOR_JOIN, &ReversibleRaceCalculator::is_race_reversible_ActorJoin},
                 {Action::BARRIER_ASYNC_LOCK, &ReversibleRaceCalculator::is_race_reversible_BarrierAsyncLock},
                 {Action::BARRIER_WAIT, &ReversibleRaceCalculator::is_race_reversible_BarrierWait},
                 {Action::COMM_ASYNC_SEND, &ReversibleRaceCalculator::is_race_reversible_CommSend},
                 {Action::COMM_ASYNC_RECV, &ReversibleRaceCalculator::is_race_reversible_CommRecv},
                 {Action::COMM_TEST, &ReversibleRaceCalculator::is_race_reversible_CommTest},
                 {Action::COMM_WAIT, &ReversibleRaceCalculator::is_race_reversible_CommWait},
                 {Action::MUTEX_ASYNC_LOCK, &ReversibleRaceCalculator::is_race_reversible_MutexAsyncLock},
                 {Action::MUTEX_TEST, &ReversibleRaceCalculator::is_race_reversible_MutexTest},
                 {Action::MUTEX_TRYLOCK, &ReversibleRaceCalculator::is_race_reversible_MutexTrylock},
                 {Action::MUTEX_UNLOCK, &ReversibleRaceCalculator::is_race_reversible_MutexUnlock},
                 {Action::MUTEX_WAIT, &ReversibleRaceCalculator::is_race_reversible_MutexWait},
                 {Action::OBJECT_ACCESS, &ReversibleRaceCalculator::is_race_reversible_ObjectAccess},
                 {Action::RANDOM, &ReversibleRaceCalculator::is_race_reversible_Random},
                 {Action::SEM_ASYNC_LOCK, &ReversibleRaceCalculator::is_race_reversible_SemAsyncLock},
                 {Action::SEM_UNLOCK, &ReversibleRaceCalculator::is_race_reversible_SemUnlock},
                 {Action::SEM_WAIT, &ReversibleRaceCalculator::is_race_reversible_SemWait},
                 {Action::TESTANY, &ReversibleRaceCalculator::is_race_reversible_TestAny},
                 {Action::WAITANY, &ReversibleRaceCalculator::is_race_reversible_WaitAny}};

  const auto* e2_action = E.get_transition_for_handle(e2);
  if (const auto handler = handlers.find(e2_action->type_); handler != handlers.end()) {
    return handler->second(E, e1, e2_action);
  } else {
    xbt_die("There is currently no specialized computation for the transition "
            "'%s' for computing reversible races in ODPOR, so the model checker cannot "
            "determine how to proceed. Please submit a bug report requesting "
            "that the transition be supported in SimGrid using ODPPR and consider "
            "using the other model-checking algorithms supported by SimGrid instead "
            "in the meantime",
            e2_action->to_string().c_str());
  }
}

bool ReversibleRaceCalculator::is_race_reversible_ActorJoin(const Execution&, Execution::EventHandle e1,
                                                            const Transition* e2)
{
  // ActorJoin races with another event iff its target `T` is the same as
  // the actor executing the other transition. Clearly, then, we could not join
  // on that actor `T` and then run a transition by `T`, so no race is reversible
  return false;
}

bool ReversibleRaceCalculator::is_race_reversible_BarrierAsyncLock(const Execution&, Execution::EventHandle e1,
                                                                   const Transition* e2)
{
  // BarrierAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_BarrierWait(const Execution& E, Execution::EventHandle e1,
                                                              const Transition* e2)
{
  // If the other event is a barrier lock event, then we
  // are not reversible; otherwise we are reversible.
  const auto e1_action = E.get_transition_for_handle(e1)->type_;
  return e1_action != Transition::Type::BARRIER_ASYNC_LOCK;
}

bool ReversibleRaceCalculator::is_race_reversible_CommRecv(const Execution&, Execution::EventHandle e1,
                                                           const Transition* e2)
{
  // CommRecv is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_CommSend(const Execution&, Execution::EventHandle e1,
                                                           const Transition* e2)
{
  // CommSend is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_CommWait(const Execution& E, Execution::EventHandle e1,
                                                           const Transition* e2)
{
  // If the other event is a communication event, then we
  // are not reversible; otherwise we are reversible.
  const auto e1_action = E.get_transition_for_handle(e1)->type_;
  return e1_action != Transition::Type::COMM_ASYNC_SEND and e1_action != Transition::Type::COMM_ASYNC_RECV;
}

bool ReversibleRaceCalculator::is_race_reversible_CommTest(const Execution&, Execution::EventHandle e1,
                                                           const Transition* e2)
{
  // CommTest is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexAsyncLock(const Execution&, Execution::EventHandle e1,
                                                                 const Transition* e2)
{
  // MutexAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexTest(const Execution&, Execution::EventHandle e1,
                                                            const Transition* e2)
{
  // MutexTest is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexTrylock(const Execution&, Execution::EventHandle e1,
                                                               const Transition* e2)
{
  // MutexTrylock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexUnlock(const Execution&, Execution::EventHandle e1,
                                                              const Transition* e2)
{
  // MutexUnlock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexWait(const Execution& E, Execution::EventHandle e1,
                                                            const Transition* e2)
{
  // TODO: Get the semantics correct here
  const auto e1_action = E.get_transition_for_handle(e1)->type_;
  return e1_action != Transition::Type::MUTEX_ASYNC_LOCK and e1_action != Transition::Type::MUTEX_UNLOCK;
}

bool ReversibleRaceCalculator::is_race_reversible_SemAsyncLock(const Execution&, Execution::EventHandle e1,
                                                               const Transition* e2)
{
  // SemAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_SemUnlock(const Execution&, Execution::EventHandle e1,
                                                            const Transition* e2)
{
  // SemUnlock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_SemWait(const Execution&, Execution::EventHandle e1,
                                                          const Transition* e2)
{
  // TODO: Get the semantics correct here
  return false;
}

bool ReversibleRaceCalculator::is_race_reversible_ObjectAccess(const Execution&, Execution::EventHandle e1,
                                                               const Transition* e2)
{
  // Object access is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_Random(const Execution&, Execution::EventHandle e1,
                                                         const Transition* e2)
{
  // Random is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_TestAny(const Execution&, Execution::EventHandle e1,
                                                          const Transition* e2)
{
  // TestAny is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_WaitAny(const Execution&, Execution::EventHandle e1,
                                                          const Transition* e2)
{
  // TODO: We need to check if any of the transitions
  // waited on occurred before `e1`
  return false;
}

} // namespace simgrid::mc::odpor
