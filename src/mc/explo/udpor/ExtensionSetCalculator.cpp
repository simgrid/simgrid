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
      HandlerMap{{Action::COMM_ASYNC_RECV, &ExtensionSetCalculator::partially_extend_CommRecv}};

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

  // TODO: if this is the first action by the actor, no such previous event exists.
  // How do we react here? Do we say we're dependent with the root event?
  const auto send_action        = std::static_pointer_cast<CommSendTransition>(std::move(action));
  const unsigned sender_mailbox = send_action->get_mailbox();
  const auto pre_event_a_C      = C.pre_event(send_action->aid_).value();

  // 1. Create `e' := <a, config(preEvt(a, C))>` and add `e'` to `ex(C)`
  const auto e_prime = U->discover_event(EventSet({pre_event_a_C}), send_action);
  exC.insert(e_prime);

  // 2. foreach e ∈ C s.t. λ(e) ∈ {AsyncSend(m, _), TestAny(Com)} where
  // Com contains a matching c' = AsyncReceive(m, _) with a
  for (const auto e : C) {
    const bool transition_type_check = [&]() {
      if (const auto* async_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
          e != nullptr && async_send->get_mailbox() == sender_mailbox) {
        return true;
      } else if (const auto* e_test_any = dynamic_cast<const CommTestTransition*>(e->get_transition());
                 e_test_any != nullptr) {
        // TODO: Check if there's a way to find a matching AsyncReceive -> matching when? in the history?
        return true;
      }
      return false;
    }();

    if (transition_type_check) {
      const EventSet K = EventSet({e, pre_event_a_C}).get_largest_maximal_subset();

      // TODO: How do we compute D_K(a, b)? There's a specialized
      // function for each case it appears, but I don't see where
      // `config(K)` comes in
      if (false) {
        const auto e_prime = U->discover_event(std::move(K), send_action);
        exC.insert(e_prime);
      }
    }
  }

  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommRecv(const Configuration& C, Unfolding* U,
                                                           const std::shared_ptr<Transition> recv_action)
{
  return EventSet();
}

EventSet ExtensionSetCalculator::partially_extend_CommWait(const Configuration&, Unfolding* U,
                                                           std::shared_ptr<Transition> wait_action)
{
  return EventSet();
}

EventSet ExtensionSetCalculator::partially_extend_CommTest(const Configuration&, Unfolding* U,
                                                           std::shared_ptr<Transition> test_action)
{
  return EventSet();
}

} // namespace simgrid::mc::udpor