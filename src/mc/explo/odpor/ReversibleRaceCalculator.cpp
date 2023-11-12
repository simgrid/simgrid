/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/odpor/ReversibleRaceCalculator.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"

#include <functional>
#include <unordered_map>
#include <xbt/asserts.h>
#include <xbt/ex.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_odpor_reversible_race, mc_dfs, "ODPOR exploration algorithm of the model-checker");

namespace simgrid::mc::odpor {

/**
   The reversible race detector should only be used if we already have the assumption
   e1 <* e2 (see Source set: a foundation for ODPOR). In particular this means that :
   - e1 -->_E e2
   - proc(e1) != proc(e2)
   - there is no event e3 s.t. e1 --> e3 --> e2
*/

bool ReversibleRaceCalculator::is_race_reversible(const Execution& E, Execution::EventHandle e1,
                                                  Execution::EventHandle e2)
{
  using Action     = Transition::Type;
  using Handler    = std::function<bool(const Execution&, const Transition*, const Transition*)>;
  using HandlerMap = std::unordered_map<Action, Handler>;

  const static HandlerMap handlers = {
      {Action::ACTOR_JOIN, &ReversibleRaceCalculator::is_race_reversible_ActorJoin},
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

  const auto* other_transition = E.get_transition_for_handle(e1);
  const auto* t2 = E.get_transition_for_handle(e2);
  if (const auto handler = handlers.find(t2->type_); handler != handlers.end()) {
    return handler->second(E, other_transition, t2);
  } else {
    xbt_die("There is currently no specialized computation for the transition "
            "'%s' for computing reversible races in ODPOR, so the model checker cannot "
            "determine how to proceed. Please submit a bug report requesting "
            "that the transition be supported in SimGrid using ODPPR and consider "
            "using the other model-checking algorithms supported by SimGrid instead "
            "in the meantime",
            t2->to_string().c_str());
  }
}

bool ReversibleRaceCalculator::is_race_reversible_ActorJoin(const Execution&, const Transition* /*other_transition*/,
                                                            const Transition* /*t2*/)
{
  // ActorJoin races with another event iff its target `T` is the same as
  // the actor executing the other transition. Clearly, then, we could not join
  // on that actor `T` and then run a transition by `T`, so no race is reversible
  return false;
}

bool ReversibleRaceCalculator::is_race_reversible_BarrierAsyncLock(const Execution&,
                                                                   const Transition* /*other_transition*/,
                                                                   const Transition* /*t2*/)
{
  // BarrierAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_BarrierWait(const Execution& E, const Transition* other_transition,
                                                              const Transition* /*t2*/)
{
  // If the other event is a barrier lock event, then we
  // are not reversible; otherwise we are reversible.
  const auto other_action = other_transition->type_;
  return other_action != Transition::Type::BARRIER_ASYNC_LOCK;
}

bool ReversibleRaceCalculator::is_race_reversible_CommRecv(const Execution&, const Transition* /*other_transition*/,
                                                           const Transition* /*t2*/)
{
  // CommRecv is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_CommSend(const Execution&, const Transition* /*other_transition*/,
                                                           const Transition* /*t2*/)
{
  // CommSend is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_CommWait(const Execution& E, const Transition* other_transition,
                                                           const Transition* /*t2*/)
{
  // If the other event is a communication event, then we
  // are not reversible; otherwise we are reversible.
  const auto other_action = other_transition->type_;
  return other_action != Transition::Type::COMM_ASYNC_SEND && other_action != Transition::Type::COMM_ASYNC_RECV;
}

bool ReversibleRaceCalculator::is_race_reversible_CommTest(const Execution&, const Transition* /*other_transition*/,
                                                           const Transition* /*t2*/)
{
  // CommTest is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexAsyncLock(const Execution&,
                                                                 const Transition* /*other_transition*/,
                                                                 const Transition* /*t2*/)
{
  // MutexAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexTest(const Execution&, const Transition* /*other_transition*/,
                                                            const Transition* /*t2*/)
{
  // MutexTest is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexTrylock(const Execution&, const Transition* /*other_transition*/,
                                                               const Transition* /*t2*/)
{
  // MutexTrylock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexUnlock(const Execution&, const Transition* /*other_transition*/,
                                                              const Transition* /*t2*/)
{
  // MutexUnlock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_MutexWait(const Execution& E, const Transition* /*other_transition*/,
                                                            const Transition* /*t2*/)
{
  // Only an Unlock can be dependent with a Wait
  // and in this case, the Unlock enbaled the wait
  // Not reversibled
  return false;
}

bool ReversibleRaceCalculator::is_race_reversible_SemAsyncLock(const Execution&, const Transition* /*other_transition*/,
                                                               const Transition* /*t2*/)
{
  // SemAsyncLock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_SemUnlock(const Execution&, const Transition* /*other_transition*/,
                                                            const Transition* /*t2*/)
{
  // SemUnlock is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_SemWait(const Execution& E, const Transition* other_transition,
                                                          const Transition* /*t2*/)
{

  if (other_transition->type_ == Transition::Type::SEM_UNLOCK &&
      static_cast<const SemaphoreTransition*>(other_transition)->get_capacity() <= 1) {
    return false;
  }
  xbt_die("SEM_WAIT that is dependent with a SEM_UNLOCK should not be reversible. FixMe");
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_ObjectAccess(const Execution&, const Transition* /*other_transition*/,
                                                               const Transition* /*t2*/)
{
  // Object access is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_Random(const Execution&, const Transition* /*other_transition*/,
                                                         const Transition* /*t2*/)
{
  // Random is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_TestAny(const Execution&, const Transition* /*other_transition*/,
                                                          const Transition* /*t2*/)
{
  // TestAny is always enabled
  return true;
}

bool ReversibleRaceCalculator::is_race_reversible_WaitAny(const Execution&, const Transition* /*other_transition*/,
                                                          const Transition* /*t2*/)
{
  // TODO: We need to check if any of the transitions
  // waited on occurred before `e1`
  return true; // Let's overapproximate to not miss branches
}

} // namespace simgrid::mc::odpor
