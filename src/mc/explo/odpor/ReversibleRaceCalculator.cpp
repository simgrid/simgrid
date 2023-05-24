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
      HandlerMap{{Action::COMM_ASYNC_RECV, &ReversibleRaceCalculator::is_race_reversible_CommRecv},
                 {Action::COMM_ASYNC_SEND, &ReversibleRaceCalculator::is_race_reversible_CommSend},
                 {Action::COMM_WAIT, &ReversibleRaceCalculator::is_race_reversible_CommWait}};

  const auto e2_action = E.get_transition_for_handle(e2);
  if (const auto handler = handlers.find(e2_action->type_); handler != handlers.end()) {
    return handler->second(E, e1, e2_action);
  } else {
    xbt_assert(false,
               "There is currently no specialized computation for the transition "
               "'%s' for computing reversible races in ODPOR, so the model checker cannot "
               "determine how to proceed. Please submit a bug report requesting "
               "that the transition be supported in SimGrid using ODPPR and consider "
               "using the other model-checking algorithms supported by SimGrid instead "
               "in the meantime",
               e2_action->to_string().c_str());
    DIE_IMPOSSIBLE;
  }
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
  // If the other event communication event, then we
  // are not reversible. Otherwise we are reversible.
  const auto e1_action = E.get_transition_for_handle(e1)->type_;
  return e1_action != Transition::Type::COMM_ASYNC_SEND and e1_action != Transition::Type::COMM_ASYNC_RECV;
}

bool ReversibleRaceCalculator::is_race_reversible_CommTest(const Execution&, Execution::EventHandle e1,
                                                           const Transition* e2)
{
  return false;
}

} // namespace simgrid::mc::odpor