/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"

#include <functional>
#include <unordered_map>
#include <xbt/asserts.h>
#include <xbt/ex.h>

using namespace simgrid::mc;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_extension, mc_udpor, "Logging specific to the extension computation in UDPOR");

namespace simgrid::mc::udpor {

EventSet ExtensionSetCalculator::partially_extend(const Configuration& C, Unfolding* U,
                                                  std::shared_ptr<Transition> action)
{
  using Action     = Transition::Type;
  using Handler    = std::function<EventSet(const Configuration&, Unfolding*, const std::shared_ptr<Transition>)>;
  using HandlerMap = std::unordered_map<Action, Handler>;

  const static HandlerMap handlers = {
      {Action::COMM_ASYNC_RECV, &ExtensionSetCalculator::partially_extend_CommRecv},
      {Action::COMM_ASYNC_SEND, &ExtensionSetCalculator::partially_extend_CommSend},
      {Action::COMM_WAIT, &ExtensionSetCalculator::partially_extend_CommWait},
      {Action::COMM_TEST, &ExtensionSetCalculator::partially_extend_CommTest},
      {Action::MUTEX_ASYNC_LOCK, &ExtensionSetCalculator::partially_extend_MutexAsyncLock},
      {Action::MUTEX_UNLOCK, &ExtensionSetCalculator::partially_extend_MutexUnlock},
      {Action::MUTEX_WAIT, &ExtensionSetCalculator::partially_extend_MutexWait},
      {Action::MUTEX_TEST, &ExtensionSetCalculator::partially_extend_MutexTest},
      {Action::ACTOR_JOIN, &ExtensionSetCalculator::partially_extend_ActorJoin}};

  if (const auto handler = handlers.find(action->type_); handler != handlers.end()) {
    return handler->second(C, U, std::move(action));
  } else {
    xbt_die("There is currently no specialized computation for the transition "
            "'%s' for computing extension sets in UDPOR, so the model checker cannot "
            "determine how to proceed. Please submit a bug report requesting "
            "that the transition be supported in SimGrid using UDPOR and consider "
            "using the other model-checking algorithms supported by SimGrid instead "
            "in the meantime",
            action->to_string().c_str());
  }
}

EventSet ExtensionSetCalculator::partially_extend_CommSend(const Configuration& C, Unfolding* U,
                                                           std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto send_action        = std::static_pointer_cast<CommSendTransition>(action);
  const auto pre_event_a_C      = C.pre_event(send_action->aid_);
  const unsigned sender_mailbox = send_action->get_mailbox();

  // 1. Create `e' := <a, config(preEvt(a, C))>` and add `e'` to `ex(C)`
  // NOTE: If `preEvt(a, C)` doesn't exist, we're effectively asking
  // about `config({})`
  if (pre_event_a_C.has_value()) {
    const auto* e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), send_action);
    exC.insert(e_prime);
  } else {
    const auto* e_prime = U->discover_event(EventSet(), send_action);
    exC.insert(e_prime);
  }

  // 2. foreach e ∈ C s.t. λ(e) ∈ {AsyncSend(m, _), TestAny(Com)} where
  // Com contains a matching c' = AsyncReceive(m, _) with `action`
  for (const auto e : C) {
    const bool transition_type_check = [&]() {
      const auto* async_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
      return async_send && async_send->get_mailbox() == sender_mailbox;
      // TODO: Add `TestAny` dependency
    }();

    if (transition_type_check) {
      EventSet K = EventSet({e, pre_event_a_C.value_or(e)}).get_largest_maximal_subset();

      // TODO: Check D_K(a, lambda(e)) (only matters in the case of CommTest)
      const auto e_prime = U->discover_event(std::move(K), send_action);
      exC.insert(e_prime);
    }
  }

  // TODO: Add `TestAny` dependency case
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommRecv(const Configuration& C, Unfolding* U,
                                                           std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto recv_action      = std::static_pointer_cast<CommRecvTransition>(action);
  const unsigned recv_mailbox = recv_action->get_mailbox();
  const auto pre_event_a_C    = C.pre_event(recv_action->aid_);

  // 1. Create `e' := <a, config(preEvt(a, C))>` and add `e'` to `ex(C)`
  if (pre_event_a_C.has_value()) {
    const auto* e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), recv_action);
    exC.insert(e_prime);
  } else {
    const auto* e_prime = U->discover_event(EventSet(), recv_action);
    exC.insert(e_prime);
  }

  // 2. foreach e ∈ C s.t. λ(e) ∈ {AsyncSend(m, _), TestAny(Com)} where
  // Com contains a matching c' = AsyncReceive(m, _) with a
  for (const auto e : C) {
    const bool transition_type_check = [&]() {
      const auto* async_recv = dynamic_cast<const CommRecvTransition*>(e->get_transition());
      return async_recv && async_recv->get_mailbox() == recv_mailbox;
      // TODO: Add `TestAny` dependency
    }();

    if (transition_type_check) {
      EventSet K = EventSet({e, pre_event_a_C.value_or(e)}).get_largest_maximal_subset();

      // TODO: Check D_K(a, lambda(e)) (ony matters in the case of TestAny)
      if (true) {
        const auto* e_prime = U->discover_event(std::move(K), recv_action);
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

  const auto wait_action   = std::static_pointer_cast<CommWaitTransition>(action);
  const auto wait_comm     = wait_action->get_comm();
  const auto pre_event_a_C = C.pre_event(wait_action->aid_);

  // Determine the _issuer_ of the communication of the `CommWait` event
  // in `C`. The issuer of the `CommWait` in `C` is the event in `C`
  // whose transition is the `CommRecv` or `CommSend` whose resulting
  // communication this `CommWait` waits on
  const auto issuer = std::find_if(C.begin(), C.end(), [&](const UnfoldingEvent* e) {
    if (const auto* e_issuer_receive = dynamic_cast<const CommRecvTransition*>(e->get_transition())) {
      return e_issuer_receive->aid_ == wait_action->aid_ && wait_comm == e_issuer_receive->get_comm();
    }

    if (const auto* e_issuer_send = dynamic_cast<const CommSendTransition*>(e->get_transition())) {
      return e_issuer_send->aid_ == wait_action->aid_ && wait_comm == e_issuer_send->get_comm();
    }

    return false;
  });
  xbt_assert(issuer != C.end(),
             "Invariant violation! A (supposedly) enabled `CommWait` transition "
             "waiting on communication %u should not be enabled: the receive/send "
             "transition which generated the communication is not an action taken "
             "to reach state(C) (the state of the configuration), which should "
             "be an impossibility if `%s` is enabled. Please report this as "
             "a bug in SimGrid's UDPOR implementation",
             wait_action->get_comm(), wait_action->to_string(false).c_str());
  const UnfoldingEvent* e_issuer = *issuer;
  const History e_issuer_history(e_issuer);

  // 1. if `a` is enabled at state(config({preEvt(a,C)})), then
  // create `e' := <a, config({preEvt(a,C)})>` and add `e'` to `ex(C)`
  //
  // First, if `pre_event_a_C == std::nullopt`, then there is nothing to
  // do: `CommWait` will never be enabled in the empty configuration (at
  // least two actions must be executed before)
  if (pre_event_a_C.has_value(); const auto* unwrapped_pre_event = pre_event_a_C.value()) {
    // A necessary condition is that the issuer be present in
    // config({preEvt(a, C)}); otherwise, the `CommWait` could not
    // be enabled since the communication on which it waits would not
    // have been created for it!
    if (const auto config_pre_event = History(unwrapped_pre_event); config_pre_event.contains(e_issuer)) {
      // If the issuer is a `CommRecv` (resp. `CommSend`), then we check that there
      // are at least as many `CommSend` (resp. `CommRecv`) transitions in `config_pre_event`
      // as needed to reach the receive/send number that is `issuer`.
      // ...
      // ...
      if (const auto* e_issuer_receive = dynamic_cast<const CommRecvTransition*>(e_issuer->get_transition())) {
        const unsigned issuer_mailbox = e_issuer_receive->get_mailbox();

        // Check from the config -> how many sends have there been
        const unsigned send_position =
            std::count_if(config_pre_event.begin(), config_pre_event.end(), [=](const auto e) {
              const auto* e_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
              return e_send && e_send->get_mailbox() == issuer_mailbox;
            });

        // Check from e_issuer -> what place is the issuer in?
        const unsigned receive_position =
            std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto e) {
              const auto* e_receive = dynamic_cast<const CommRecvTransition*>(e->get_transition());
              return e_receive && e_receive->get_mailbox() == issuer_mailbox;
            });

        if (send_position >= receive_position) {
          exC.insert(U->discover_event(EventSet({unwrapped_pre_event}), wait_action));
        }

      } else if (const auto* e_issuer_send = dynamic_cast<const CommSendTransition*>(e_issuer->get_transition())) {
        const unsigned issuer_mailbox = e_issuer_send->get_mailbox();

        // Check from e_issuer -> what place is the issuer in?
        const unsigned send_position =
            std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto e) {
              const auto* e_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
              return e_send && e_send->get_mailbox() == issuer_mailbox;
            });

        // Check from the config -> how many sends have there been
        const unsigned receive_position =
            std::count_if(config_pre_event.begin(), config_pre_event.end(), [=](const auto e) {
              const auto* e_receive = dynamic_cast<const CommRecvTransition*>(e->get_transition());
              return e_receive && e_receive->get_mailbox() == issuer_mailbox;
            });

        if (send_position <= receive_position) {
          exC.insert(U->discover_event(EventSet({unwrapped_pre_event}), wait_action));
        }

      } else {
        xbt_die("The transition which created the communication on which `%s` waits "
                "is neither an async send nor an async receive. The current UDPOR "
                "implementation does not know how to check if `CommWait` is enabled in "
                "this case. Was a new transition added?",
                e_issuer->get_transition()->to_string().c_str());
      }
    }
  }

  // 3. foreach event e in C do
  if (const auto* e_issuer_send = dynamic_cast<const CommSendTransition*>(e_issuer->get_transition())) {
    for (const auto e : C) {
      // If the provider of the communication for `CommWait` is a
      // `CommSend(m)`, then we only care about `e` if `λ(e) == `CommRecv(m)`.
      // All other actions would be independent with the wait action (including
      // another `CommSend` to the same mailbox: `CommWait` is "waiting" for its
      // corresponding receive action)
      if (e->get_transition()->type_ != Transition::Type::COMM_ASYNC_RECV) {
        continue;
      }

      const auto issuer_mailbox = e_issuer_send->get_mailbox();
      if (const auto* e_recv = dynamic_cast<const CommRecvTransition*>(e->get_transition());
          e_recv->get_mailbox() != issuer_mailbox or e_recv->get_comm() != wait_action->get_comm()) {
        continue;
      }

      // If the `issuer` is not in `config(K)`, this implies that
      // `WaitAny()` is always disabled in `config(K)`; hence, it
      // is independent of any transition in `config(K)` (according
      // to formal definition of independence)
      auto K              = EventSet({e, pre_event_a_C.value_or(e)});
      const auto config_K = History(K);
      if (not config_K.contains(e_issuer)) {
        continue;
      }

      // What send # is the issuer
      const unsigned send_position =
          std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto ev) {
            const auto* e_send = dynamic_cast<const CommSendTransition*>(ev->get_transition());
            return e_send && e_send->get_mailbox() == issuer_mailbox;
          });

      // What receive # is the event `e`?
      const unsigned receive_position = std::count_if(config_K.begin(), config_K.end(), [=](const auto ev) {
        const auto* e_receive = dynamic_cast<const CommRecvTransition*>(ev->get_transition());
        return e_receive && e_receive->get_mailbox() == issuer_mailbox;
      });

      if (send_position == receive_position) {
        exC.insert(U->discover_event(std::move(K), wait_action));
      }
    }
  } else if (const auto* e_issuer_recv = dynamic_cast<const CommRecvTransition*>(e_issuer->get_transition())) {
    for (const auto e : C) {
      // If the provider of the communication for `CommWait` is a
      // `CommRecv(m)`, then we only care about `e` if `λ(e) == `CommSend(m)`.
      // All other actions would be independent with the wait action (including
      // another `CommRecv` to the same mailbox: `CommWait` is "waiting" for its
      // corresponding send action)
      if (e->get_transition()->type_ != Transition::Type::COMM_ASYNC_SEND) {
        continue;
      }

      const auto issuer_mailbox = e_issuer_recv->get_mailbox();
      if (const auto* e_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
          e_send->get_mailbox() != issuer_mailbox or e_send->get_comm() != wait_action->get_comm()) {
        continue;
      }

      // If the `issuer` is not in `config(K)`, this implies that
      // `WaitAny()` is always disabled in `config(K)`; hence, it
      // is independent of any transition in `config(K)` (according
      // to formal definition of independence)
      auto K              = EventSet({e, pre_event_a_C.value_or(e)});
      const auto config_K = History(K);
      if (not config_K.contains(e_issuer)) {
        continue;
      }

      // What receive # is the event `e`?
      const unsigned send_position = std::count_if(config_K.begin(), config_K.end(), [=](const auto ev) {
        const auto* e_send = dynamic_cast<const CommSendTransition*>(ev->get_transition());
        return e_send && e_send->get_mailbox() == issuer_mailbox;
      });

      // What send # is the issuer
      const unsigned receive_position =
          std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto ev) {
            const auto* e_receive = dynamic_cast<const CommRecvTransition*>(ev->get_transition());
            return e_receive && e_receive->get_mailbox() == issuer_mailbox;
          });

      if (send_position == receive_position) {
        exC.insert(U->discover_event(std::move(K), wait_action));
      }
    }
  } else {
    xbt_die("The transition which created the communication on which `%s` waits "
            "is neither an async send nor an async receive. The current UDPOR "
            "implementation does not know how to check if `CommWait` is enabled in "
            "this case. Was a new transition added?",
            e_issuer->get_transition()->to_string().c_str());
  }

  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_CommTest(const Configuration& C, Unfolding* U,
                                                           std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto test_action   = std::static_pointer_cast<CommTestTransition>(action);
  const auto test_comm     = test_action->get_comm();
  const auto test_aid      = test_action->aid_;
  const auto pre_event_a_C = C.pre_event(test_action->aid_);

  // Add the previous event as a dependency (if it's there)
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), test_action);
    exC.insert(e_prime);
  }

  // Determine the _issuer_ of the communication of the `CommTest` event
  // in `C`. The issuer of the `CommTest` in `C` is the event in `C`
  // whose transition is the `CommRecv` or `CommSend` whose resulting
  // communication this `CommTest` tests on
  const auto issuer = std::find_if(C.begin(), C.end(), [=](const UnfoldingEvent* e) {
    if (const auto* e_issuer_receive = dynamic_cast<const CommRecvTransition*>(e->get_transition())) {
      return e_issuer_receive->aid_ == test_aid && test_comm == e_issuer_receive->get_comm();
    }

    if (const auto* e_issuer_send = dynamic_cast<const CommSendTransition*>(e->get_transition())) {
      return e_issuer_send->aid_ == test_aid && test_comm == e_issuer_send->get_comm();
    }

    return false;
  });
  xbt_assert(issuer != C.end(),
             "An enabled `CommTest` transition (%s) is testing a communication"
             "%u not created by a receive/send "
             "transition. SimGrid cannot currently handle test actions "
             "under which a test is performed on a communication that was "
             "not directly created by a receive/send operation of the same actor.",
             test_action->to_string(false).c_str(), test_action->get_comm());
  const UnfoldingEvent* e_issuer = *issuer;
  const History e_issuer_history(e_issuer);

  // 3. foreach event e in C do
  if (const auto* e_issuer_send = dynamic_cast<const CommSendTransition*>(e_issuer->get_transition())) {
    for (const auto e : C) {
      // If the provider of the communication for `CommTest` is a
      // `CommSend(m)`, then we only care about `e` if `λ(e) == `CommRecv(m)`.
      // All other actions would be independent with the test action (including
      // another `CommSend` to the same mailbox: `CommTest` is testing the
      // corresponding receive action)
      if (e->get_transition()->type_ != Transition::Type::COMM_ASYNC_RECV) {
        continue;
      }

      const auto issuer_mailbox = e_issuer_send->get_mailbox();

      if (const auto* e_recv = dynamic_cast<const CommRecvTransition*>(e->get_transition());
          e_recv->get_mailbox() != issuer_mailbox) {
        continue;
      }

      // If the `issuer` is not in `config(K)`, this implies that
      // `CommTest()` is always disabled in `config(K)`; hence, it
      // is independent of any transition in `config(K)` (according
      // to formal definition of independence)
      auto K              = EventSet({e, pre_event_a_C.value_or(e)});
      const auto config_K = History(K);
      if (not config_K.contains(e_issuer)) {
        continue;
      }

      // What send # is the issuer
      const unsigned send_position =
          std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto ev) {
            const auto* e_send = dynamic_cast<const CommSendTransition*>(ev->get_transition());
            return e_send && e_send->get_mailbox() == issuer_mailbox;
          });

      // What receive # is the event `e`?
      const unsigned receive_position = std::count_if(config_K.begin(), config_K.end(), [=](const auto ev) {
        const auto* e_receive = dynamic_cast<const CommRecvTransition*>(ev->get_transition());
        return e_receive && e_receive->get_mailbox() == issuer_mailbox;
      });

      if (send_position == receive_position) {
        exC.insert(U->discover_event(std::move(K), test_action));
      }
    }
  } else if (const auto* e_issuer_recv = dynamic_cast<const CommRecvTransition*>(e_issuer->get_transition())) {
    for (const auto e : C) {
      // If the provider of the communication for `CommTest` is a
      // `CommRecv(m)`, then we only care about `e` if `λ(e) == `CommSend(m)`.
      // All other actions would be independent with the wait action (including
      // another `CommRecv` to the same mailbox: `CommWait` is "waiting" for its
      // corresponding send action)
      if (e->get_transition()->type_ != Transition::Type::COMM_ASYNC_SEND) {
        continue;
      }

      const auto issuer_mailbox = e_issuer_recv->get_mailbox();
      if (const auto* e_send = dynamic_cast<const CommSendTransition*>(e->get_transition());
          e_send->get_mailbox() != issuer_mailbox) {
        continue;
      }

      // If the `issuer` is not in `config(K)`, this implies that
      // `WaitAny()` is always disabled in `config(K)`; hence, it
      // is independent of any transition in `config(K)` (according
      // to formal definition of independence)
      auto K              = EventSet({e, pre_event_a_C.value_or(e)});
      const auto config_K = History(K);
      if (not config_K.contains(e_issuer)) {
        continue;
      }

      // What receive # is the event `e`?
      const unsigned send_position = std::count_if(config_K.begin(), config_K.end(), [=](const auto ev) {
        const auto* e_send = dynamic_cast<const CommSendTransition*>(ev->get_transition());
        return e_send && e_send->get_mailbox() == issuer_mailbox;
      });

      // What send # is the issuer
      const unsigned receive_position =
          std::count_if(e_issuer_history.begin(), e_issuer_history.end(), [=](const auto ev) {
            const auto* e_receive = dynamic_cast<const CommRecvTransition*>(ev->get_transition());
            return e_receive && e_receive->get_mailbox() == issuer_mailbox;
          });

      if (send_position == receive_position) {
        exC.insert(U->discover_event(std::move(K), test_action));
      }
    }
  } else {
    xbt_die("The transition which created the communication on which `%s` waits "
            "is neither an async send nor an async receive. The current UDPOR "
            "implementation does not know how to check if `CommWait` is enabled in "
            "this case. Was a new transition added?",
            e_issuer->get_transition()->to_string().c_str());
  }
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_MutexAsyncLock(const Configuration& C, Unfolding* U,
                                                                 std::shared_ptr<Transition> action)
{
  EventSet exC;
  const auto mutex_lock    = std::static_pointer_cast<MutexTransition>(action);
  const auto pre_event_a_C = C.pre_event(mutex_lock->aid_);

  // for each event e in C
  // 1. If lambda(e) := pre(a) -> add it. Note that if
  // pre_event_a_C.has_value() == false, this implies `C` is
  // empty or which we treat as implicitly containing the bottom event
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), mutex_lock);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet(), mutex_lock);
    exC.insert(e_prime);
  }

  // for each event e in C
  for (const auto e : C) {
    // Check for other locks on the same mutex, but not in the same aid history
    if (const auto* e_mutex = dynamic_cast<const MutexTransition*>(e->get_transition());
        e_mutex != nullptr && e_mutex->type_ == Transition::Type::MUTEX_ASYNC_LOCK &&
        mutex_lock->get_mutex() == e_mutex->get_mutex() && mutex_lock->aid_ != e_mutex->aid_) {

      auto K = EventSet();
      if (!pre_event_a_C.has_value() or not e->in_history_of(pre_event_a_C.value()))
        K.insert(e);
      if (pre_event_a_C.has_value() and not pre_event_a_C.value()->in_history_of(e))
        K.insert(pre_event_a_C.value());
      if (!K.empty())
        exC.insert(U->discover_event(std::move(K), mutex_lock));
    }
  }
  return exC;
}

std::pair<aid_t, aid_t> firstTwoOwners(uintptr_t mutex_id, EventSet history)
{
  std::pair<aid_t, aid_t> two_owners(-1, -1);
  for (const auto e : history) {
    if (e->get_transition()->type_ == Transition::Type::MUTEX_ASYNC_LOCK) {
      if (two_owners.first == -1)
        two_owners.first = e->get_transition()->aid_;
      else if (two_owners.second == -1) {
        two_owners.second = e->get_transition()->aid_;
        return two_owners;
      }
    }
  }
  return two_owners;
}

EventSet ExtensionSetCalculator::partially_extend_MutexUnlock(const Configuration& C, Unfolding* U,
                                                              std::shared_ptr<Transition> action)
{
  EventSet exC;
  const auto mutex_unlock  = std::static_pointer_cast<MutexTransition>(action);
  const auto pre_event_a_C = C.pre_event(mutex_unlock->aid_);

  // for each event e in C
  // 1. If lambda(e) := pre(a) -> add it. Note that if
  // pre_event_a_C.has_value() == false, this implies `C` is
  // empty or which we treat as implicitly containing the bottom event
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), mutex_unlock);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet(), mutex_unlock);
    exC.insert(e_prime);
  }

  // for each event e in C
  for (const auto e : C) {
    // Check for MutexTest
    if (const auto* e_mutex = dynamic_cast<const MutexTransition*>(e->get_transition());
        e_mutex != nullptr &&
        (e_mutex->type_ == Transition::Type::MUTEX_TEST || e_mutex->type_ == Transition::Type::MUTEX_WAIT) &&
        e_mutex->get_mutex() == mutex_unlock->get_mutex() && mutex_unlock->aid_ != e_mutex->aid_) {

      auto first_owners = firstTwoOwners(e_mutex->get_mutex(), pre_event_a_C.value()->get_history());
      if (first_owners == std::pair(mutex_unlock->aid_, e_mutex->aid_) or
          first_owners == std::pair(e_mutex->aid_, mutex_unlock->aid_)) {

        auto K = EventSet();
        if (!pre_event_a_C.has_value() or not e->in_history_of(pre_event_a_C.value()))
          K.insert(e);
        if (pre_event_a_C.has_value() and not pre_event_a_C.value()->in_history_of(e))
          K.insert(pre_event_a_C.value());
        if (!K.empty())
          exC.insert(U->discover_event(std::move(K), mutex_unlock));
      }
    }
  }
  return exC;
}

bool is_mutex_available_before(const UnfoldingEvent* e, std::shared_ptr<MutexTransition> mutex)
{
  XBT_DEBUG("Wondering if the mutex is available just after %s history", e->to_string().c_str());
  unsigned long requests_over_mutex = 0;

  auto* previous_mutex = dynamic_cast<MutexTransition*>(e->get_transition());
  if (previous_mutex != nullptr && e->get_transition()->type_ == Transition::Type::MUTEX_ASYNC_LOCK &&
      mutex->depends(e->get_transition()))
    requests_over_mutex++;

  for (const auto previous_event : e->get_history()) {
    XBT_DEBUG("  Considering event %s in the history", previous_event->to_string().c_str());
    auto* previous_mutex = dynamic_cast<MutexTransition*>(previous_event->get_transition());
    if (previous_mutex == nullptr)
      continue;

    if (previous_event->get_transition()->type_ == Transition::Type::MUTEX_ASYNC_LOCK &&
        mutex->get_mutex() == previous_mutex->get_mutex())
      requests_over_mutex++;

    if (previous_event->get_transition()->type_ == Transition::Type::MUTEX_UNLOCK &&
        mutex->get_mutex() == previous_mutex->get_mutex())
      requests_over_mutex--;
  }
  XBT_DEBUG("  Mutex requests : %lu", requests_over_mutex);
  // There will always be the AsyncLock by this actor
  // mutex is owned if it is the only "unmatched" request
  return requests_over_mutex == 1;
}

EventSet ExtensionSetCalculator::partially_extend_MutexWait(const Configuration& C, Unfolding* U,
                                                            std::shared_ptr<Transition> action)
{
  EventSet exC;
  const auto mutex_wait    = std::static_pointer_cast<MutexTransition>(action);
  const auto pre_event_a_C = C.pre_event(mutex_wait->aid_);
  xbt_assert(pre_event_a_C.has_value());

  // for each event e in C
  // 1. If lambda(e) := pre(a) -> add it. In the case of MutexWait, we also check that the
  // actor which is executing the MutexWait is the owner of the mutex
  if (is_mutex_available_before(pre_event_a_C.value(), mutex_wait)) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), mutex_wait);
    exC.insert(e_prime);
  }

  // for each event e in C
  for (const auto e : C) {
    // Check for any unlocks
    if (const auto* e_mutex = dynamic_cast<const MutexTransition*>(e->get_transition());
        e_mutex != nullptr && e_mutex->type_ == Transition::Type::MUTEX_UNLOCK &&
        mutex_wait->get_mutex() == e_mutex->get_mutex() && mutex_wait->aid_ != e_mutex->aid_) {

      auto first_owners = firstTwoOwners(e_mutex->get_mutex(), pre_event_a_C.value()->get_history());
      if (first_owners == std::pair(mutex_wait->aid_, e_mutex->aid_) or
          first_owners == std::pair(e_mutex->aid_, mutex_wait->aid_)) {

        auto K = EventSet();
        if (!pre_event_a_C.has_value() or not e->in_history_of(pre_event_a_C.value()))
          K.insert(e);
        if (pre_event_a_C.has_value() and not pre_event_a_C.value()->in_history_of(e))
          K.insert(pre_event_a_C.value());
        if (!K.empty())
          exC.insert(U->discover_event(std::move(K), mutex_wait));
      }
    }
  }
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_MutexTest(const Configuration& C, Unfolding* U,
                                                            std::shared_ptr<Transition> action)
{
  EventSet exC;
  const auto mutex_test    = std::static_pointer_cast<MutexTransition>(action);
  const auto pre_event_a_C = C.pre_event(mutex_test->aid_);

  // for each event e in C
  // 1. If lambda(e) := pre(a) -> add it. Note that if
  // pre_event_a_C.has_value() == false, this implies `C` is
  // empty or which we treat as implicitly containing the bottom event
  if (pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value()}), mutex_test);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet(), mutex_test);
    exC.insert(e_prime);
  }

  // for each event e in C
  for (const auto e : C) {
    // Check for any unlocks
    if (const auto* e_mutex = dynamic_cast<const MutexTransition*>(e->get_transition());
        e_mutex != nullptr && e_mutex->type_ == Transition::Type::MUTEX_UNLOCK) {
      // TODO: Check if dependent or not
      // This entails getting information about
      // the relative position of the mutex in the queue, which
      // again means we need more context...
      auto K = EventSet({e, pre_event_a_C.value_or(e)});
      exC.insert(U->discover_event(std::move(K), mutex_test));
    }
  }
  return exC;
}

EventSet ExtensionSetCalculator::partially_extend_ActorJoin(const Configuration& C, Unfolding* U,
                                                            std::shared_ptr<Transition> action)
{
  EventSet exC;

  const auto join_action = std::static_pointer_cast<ActorJoinTransition>(action);

  const auto last_event_waited = C.pre_event(join_action->get_target());
  xbt_assert(last_event_waited.has_value(), "We considered the extension of an ActorJoin waiting for a process"
                                            "that has not yet been executed. Fix me");

  // Handling ActorJoin is very simple: it is dependent only with previous actions
  // of the same actor, and actions of the actor it is waiting for.
  if (const auto pre_event_a_C = C.pre_event(join_action->aid_); pre_event_a_C.has_value()) {
    const auto e_prime = U->discover_event(EventSet({pre_event_a_C.value(), last_event_waited.value()}), join_action);
    exC.insert(e_prime);
  } else {
    const auto e_prime = U->discover_event(EventSet({last_event_waited.value()}), join_action);
    exC.insert(e_prime);
  }

  return exC;
}

} // namespace simgrid::mc::udpor
