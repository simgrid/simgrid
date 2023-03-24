/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"

#include <functional>
#include <unordered_map>
#include <xbt/asserts.h>
#include <xbt/ex.h>

using namespace simgrid::mc;

namespace simgrid::mc::udpor {

EventSet ExtensionSetCalculator::partially_extend(const Configuration& C, Unfolding* U,
                                                  const std::shared_ptr<Transition> action)
{
  using Action     = Transition::Type;
  using Handler    = std::function<EventSet(const Configuration&, Unfolding*, const std::shared_ptr<Transition>)>;
  using HandlerMap = std::unordered_map<Action, Handler>;

  const static HandlerMap handlers =
      HandlerMap{{Action::COMM_ASYNC_RECV, &ExtensionSetCalculator::partially_extend_CommRecv},
                 {Action::COMM_ASYNC_SEND, &ExtensionSetCalculator::partially_extend_CommSend}};

  if (const auto handler = handlers.find(action->type_); handler != handlers.end()) {
    return handler->second(C, U, std::move(action));
  } else {
    xbt_assert(false,
               "There is currently no specialized computation for the transition "
               "'%s' for computing extension sets in UDPOR, so the model checker cannot "
               "determine how to proceed. Please submit a bug report requesting "
               "that the transition be supported in SimGrid using UDPOR and consider "
               "using the other model-checking algorithms supported by SimGrid instead "
               "in the meantime",
               action->to_string().c_str());
    DIE_IMPOSSIBLE;
  }
}

EventSet ExtensionSetCalculator::partially_extend_CommSend(const Configuration& C, Unfolding* U,
                                                           const std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto send_action        = std::static_pointer_cast<CommSendTransition>(std::move(action));
  const auto pre_event_a_C      = C.pre_event(send_action->aid_);
  const unsigned sender_mailbox = send_action->get_mailbox();

  // 1. Create `e' := <a, config(preEvt(a, C))>` and add `e'` to `ex(C)`
  // NOTE: If `preEvt(a, C)` doesn't exist, we're effectively asking
  // about `config({})`
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), send_action);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet(), send_action);
    exC.insert(e_prime);
  }

  // 2. foreach e ∈ C s.t. λ(e) ∈ {AsyncSend(m, _), TestAny(Com)} where
  // Com contains a matching c' = AsyncReceive(m, _) with a
  for (const auto e : C) {
    const bool transition_type_check = [&]() {
      if (const auto* async_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
          async_send != nullptr) {
        return async_send->get_mailbox() == sender_mailbox;
      }
      // TODO: Add `TestAny` dependency
      return false;
    }();

    if (transition_type_check) {
      const EventSet K = EventSet({e, pre_event_a_C.value_or(e)}).get_largest_maximal_subset();

      // TODO: Check D_K(a, lambda(e))
      if (true) {
        const auto e_prime = U->discover_event(std::move(K), send_action);
        exC.insert(e_prime);
      }
    }
  }

  // TODO: Add `TestAny` dependency case
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommRecv(const Configuration& C, Unfolding* U,
                                                           const std::shared_ptr<Transition> action)
{
  EventSet exC;

  // TODO: if this is the first action by the actor, no such previous event exists.
  // How do we react here? Do we say we're dependent with the root event?
  const auto recv_action      = std::static_pointer_cast<CommRecvTransition>(std::move(action));
  const unsigned recv_mailbox = recv_action->get_mailbox();
  const auto pre_event_a_C    = C.pre_event(recv_action->aid_);

  // 1. Create `e' := <a, config(preEvt(a, C))>` and add `e'` to `ex(C)`
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), recv_action);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet(), recv_action);
    exC.insert(e_prime);
  }

  // 2. foreach e ∈ C s.t. λ(e) ∈ {AsyncSend(m, _), TestAny(Com)} where
  // Com contains a matching c' = AsyncReceive(m, _) with a
  for (const auto e : C) {
    const bool transition_type_check = [&]() {
      if (const auto* async_recv = dynamic_cast<const CommSendTransition*>(e->get_transition());
          async_recv != nullptr && async_recv->get_mailbox() == recv_mailbox) {
        return true;
      }
      // TODO: Add `TestAny` dependency
      return false;
    }();

    if (transition_type_check) {
      const EventSet K = EventSet({e, pre_event_a_C.value_or(e)}).get_largest_maximal_subset();

      // TODO: Check D_K(a, lambda(e))
      if (true) {
        const auto e_prime = U->discover_event(std::move(K), recv_action);
        exC.insert(e_prime);
      }
    }
  }

  // TODO: Add `TestAny` dependency case
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommWait(const Configuration& C, Unfolding* U,
                                                           std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto wait_action   = std::static_pointer_cast<CommWaitTransition>(std::move(action));
  const auto pre_event_a_C = C.pre_event(wait_action->aid_);

  // 1. if `a` is enabled at state(config({preEvt(a,C)})), then
  // create `e' := <a, config({preEvt(a,C)})>` and add `e'` to `ex(C)`
  //
  // First, if `pre_event_a_C == std::nullopt`, then there is nothing to
  // do: `CommWait` will never be enabled in the empty configuration (at
  // least two actions must be executed before)
  if (pre_event_a_C.has_value(); const auto unwrapped_pre_event = pre_event_a_C.value()) {

    // Determine the _issuer_ of the communication of the `CommWait` event
    // in `C`. The issuer of the `CommWait` in `C` is the event in `C`
    // whose transition is the `CommRecv` or `CommSend` whose resutling
    // communication this `CommWait` waits on
    const auto issuer = std::find_if(C.begin(), C.end(), [=](const UnfoldingEvent* e) { return false; });
    xbt_assert(issuer != C.end(),
               "Invariant violation! A (supposedly) enabled `CommWait` transition "
               "waiting on commiunication %lu should not be enabled: the receive/send "
               "transition which generated the communication is not an action taken "
               "to reach state(C) (the state of the configuration), which should "
               "be an impossibility if `%s` is enabled. Please report this as "
               "a bug in SimGrid's UDPOR implementation",
               wait_action->get_comm(), wait_action->to_string(false).c_str());

    // A necessary condition is that the issuer be present in
    // config({preEvt(a, C)}); otherwise, the `CommWait` could not
    // be enabled since the communication on which it waits would not
    // have been created for it!
    if (const auto config_pre_event = History(unwrapped_pre_event); config_pre_event.contains(*issuer)) {
      // If the issuer is a `CommRecv` (resp. `CommSend`), then we check that there
      // are at least as many `CommSend` (resp. `CommRecv`) transitions in `config_pre_event`
      // as needed to reach the receive/send number that is `issuer`.
      //...
      //...
    }
  }

  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommTest(const Configuration& C, Unfolding* U,
                                                           std::shared_ptr<Transition> test_action)
{
  return EventSet();
}

} // namespace simgrid::mc::udpor