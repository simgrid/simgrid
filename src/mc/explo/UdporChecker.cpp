/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/UdporChecker.hpp"
#include "src/mc/api/State.hpp"
#include "src/mc/explo/udpor/Comb.hpp"
#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"

#include <numeric>
#include <xbt/asserts.h>
#include <xbt/log.h>
#include <xbt/string.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to verification using UDPOR");

namespace simgrid::mc::udpor {

UdporChecker::UdporChecker(const std::vector<char*>& args) : Exploration(args) {}

void UdporChecker::run()
{
  XBT_INFO("Starting a UDPOR exploration");
  state_stack.clear();
  state_stack.push_back(get_current_state());
  explore(Configuration(), EventSet(), EventSet(), EventSet());
  XBT_INFO("UDPOR exploration terminated -- model checking completed");
}

void UdporChecker::explore(const Configuration& C, EventSet D, EventSet A, EventSet prev_exC)
{
  auto& stateC   = *state_stack.back();
  auto exC       = compute_exC(C, stateC, prev_exC);
  const auto enC = compute_enC(C, exC);
  XBT_DEBUG("explore(C, D, A) with:\n"
            "C\t := %s \n"
            "D\t := %s \n"
            "A\t := %s \n"
            "ex(C)\t := %s \n"
            "en(C)\t := %s \n",
            C.to_string().c_str(), D.to_string().c_str(), A.to_string().c_str(), exC.to_string().c_str(),
            enC.to_string().c_str());
  XBT_DEBUG("ex(C) has %zu elements, of which %zu are in en(C)", exC.size(), enC.size());

  // If enC is a subset of D, intuitively
  // there aren't any enabled transitions
  // which are "worth" exploring since their
  // exploration would lead to a so-called
  // "sleep-set blocked" trace.
  if (enC.is_subset_of(D)) {
    XBT_DEBUG("en(C) is a subset of the sleep set D (size %zu); if we "
              "kept exploring, we'd hit a sleep-set blocked trace",
              D.size());
    XBT_DEBUG("The current configuration has %zu elements", C.get_events().size());

    // When `en(C)` is empty, intuitively this means that there
    // are no enabled transitions that can be executed from the
    // state reached by `C` (denoted `state(C)`), i.e. by some
    // execution of the transitions in C obeying the causality
    // relation. Here, then, we may be in a deadlock (the other
    // possibility is that we've finished running everything, and
    // we wouldn't be in deadlock then)
    if (enC.empty()) {
      XBT_VERB("**************************");
      XBT_VERB("*** TRACE INVESTIGATED ***");
      XBT_VERB("**************************");
      XBT_VERB("Execution sequence:");
      for (auto const& s : get_textual_trace())
        XBT_VERB("  %s", s.c_str());
      get_remote_app().check_deadlock();
    }

    return;
  }
  UnfoldingEvent* e = select_next_unfolding_event(A, enC);
  xbt_assert(e != nullptr, "\n\n****** INVARIANT VIOLATION ******\n"
                           "UDPOR guarantees that an event will be chosen at each point in\n"
                           "the search, yet no events were actually chosen\n"
                           "*********************************\n\n");
  XBT_DEBUG("Selected event `%s` (%zu dependencies) to extend the configuration", e->to_string().c_str(),
            e->get_immediate_causes().size());

  // Ce := C + {e}
  Configuration Ce = C;
  Ce.add_event(e);

  A.remove(e);
  exC.remove(e);

  // Explore(C + {e}, D, A \ {e})

  // Move the application into stateCe (i.e. `state(C + {e})`) and make note of that state
  move_to_stateCe(&stateC, e);
  state_stack.push_back(record_current_state());

  explore(Ce, D, std::move(A), std::move(exC));

  // Prepare to move the application back one state.
  // We need only remove the state from the stack here: if we perform
  // another `Explore()` after computing an alternative, at that
  // point we'll actually create a fresh RemoteProcess
  state_stack.pop_back();

  // D <-- D + {e}
  D.insert(e);

  XBT_DEBUG("Checking for the existence of an alternative...");
  if (auto J = C.compute_alternative_to(D, this->unfolding); J.has_value()) {
    // Before searching the "right half", we need to make
    // sure the program actually reflects the fact
    // that we are searching again from `state(C)`. While the
    // stack of states is properly adjusted to represent
    // `state(C)` all together, the RemoteApp is currently sitting
    // at some *future* state with respect to `state(C)` since the
    // recursive calls had moved it there.
    restore_program_state_with_current_stack();

    // Explore(C, D + {e}, J \ C)
    auto J_minus_C = J.value().get_events().subtracting(C.get_events());

    XBT_DEBUG("Alternative detected! The alternative is:\n"
              "J\t := %s \n"
              "J / C := %s\n"
              "UDPOR is going to explore it...",
              J.value().to_string().c_str(), J_minus_C.to_string().c_str());
    explore(C, D, std::move(J_minus_C), std::move(prev_exC));
  } else {
    XBT_DEBUG("No alternative detected with:\n"
              "C\t := %s \n"
              "D\t := %s \n"
              "A\t := %s \n",
              C.to_string().c_str(), D.to_string().c_str(), A.to_string().c_str());
  }

  // D <-- D - {e}
  D.remove(e);

  // Remove(e, C, D)
  clean_up_explore(e, C, D);
}

EventSet UdporChecker::compute_exC(const Configuration& C, const State& stateC, const EventSet& prev_exC)
{
  // See eqs. 5.7 of section 5.2 of [3]
  // C = C' + {e_cur}, i.e. C' = C - {e_cur}
  //
  // Then
  //
  // ex(C) = ex(C' + {e_cur}) = ex(C') / {e_cur} +
  //    U{<a, K> : K is maximal, `a` depends on all of K, `a` enabled at config(K) }
  const UnfoldingEvent* e_cur = C.get_latest_event();
  EventSet exC                = prev_exC;
  exC.remove(e_cur);

  // IMPORTANT NOTE: In order to have deterministic results, we need to process
  // the actors in a deterministic manner so that events are discovered by
  // UDPOR in a deterministic order. The processing done here always processes
  // actors in a consistent order since `std::map` is by-default ordered using
  // `std::less<Key>` (see the return type of `State::get_actors_list()`)
  for (const auto& [aid, actor_state] : stateC.get_actors_list()) {
    const auto& enabled_transitions = actor_state.get_enabled_transitions();
    if (enabled_transitions.empty()) {
      XBT_DEBUG("\t Actor `%ld` is disabled: no partial extensions need to be considered", aid);
    } else {
      XBT_DEBUG("\t Actor `%ld` is enabled", aid);
      for (const auto& transition : enabled_transitions) {
        XBT_DEBUG("\t Considering partial extension for %s", transition->to_string().c_str());
        EventSet extension = ExtensionSetCalculator::partially_extend(C, &unfolding, transition);
        exC.form_union(extension);
      }
    }
  }
  return exC;
}

EventSet UdporChecker::compute_enC(const Configuration& C, const EventSet& exC) const
{
  EventSet enC;
  for (const auto* e : exC) {
    if (C.is_compatible_with(e)) {
      enC.insert(e);
    }
  }
  return enC;
}

void UdporChecker::move_to_stateCe(State* state, UnfoldingEvent* e)
{
  const aid_t next_actor = e->get_transition()->aid_;

  // TODO: Add the trace if possible for reporting a bug
  xbt_assert(next_actor >= 0, "\n\n****** INVARIANT VIOLATION ******\n"
                              "In reaching this execution path, UDPOR ensures that at least one\n"
                              "one transition of the state of an visited event is enabled, yet no\n"
                              "state was actually enabled. Please report this as a bug.\n"
                              "*********************************\n\n");
  auto latest_transition_by_next_actor = state->execute_next(next_actor, get_remote_app());

  // The transition that is associated with the event was just
  // executed, so it's possible that the new version of the transition
  // (i.e. the one after execution) has *more* information than
  // that which existed *prior* to execution.
  //
  //
  // ------- !!!!! UDPOR INVARIANT !!!!! -------
  //
  // At this point, we are leveraging the fact that
  // UDPOR will not contain more than one copy of any
  // transition executed by any actor for any
  // particular step taken by that actor. That is,
  // if transition `i` of the `j`th actor is contained in the
  // configuration `C` currently under consideration
  // by UDPOR, then only one and only one copy exists in `C`
  //
  // This means that we can referesh the transitions associated
  // with each event lazily, i.e. only after we have chosen the
  // event to continue our execution.
  e->set_transition(std::move(latest_transition_by_next_actor));
}

void UdporChecker::restore_program_state_with_current_stack()
{
  XBT_DEBUG("Restoring state using the current stack");
  get_remote_app().restore_initial_state();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (const std::unique_ptr<State>& state : state_stack) {
    if (state == state_stack.back()) /* If we are arrived on the target state, don't replay the outgoing transition */
      break;
    state->get_transition_out()->replay(get_remote_app());
  }
}

std::unique_ptr<State> UdporChecker::record_current_state()
{
  auto next_state = this->get_current_state();

  // In UDPOR, we care about all enabled transitions in a given state
  next_state->consider_all();

  return next_state;
}

UnfoldingEvent* UdporChecker::select_next_unfolding_event(const EventSet& A, const EventSet& enC)
{
  if (enC.empty()) {
    throw std::invalid_argument("There are no unfolding events to select. "
                                "Are you sure that you checked that en(C) was not "
                                "empty before attempting to select an event from it?");
  }

  // UDPOR's exploration is non-deterministic (as is DPOR's)
  // in the sense that at any given point there may
  // be multiple paths that can be followed. The correctness and optimality
  // of the algorithm remains unaffected by the route taken by UDPOR when
  // given multiple choices; but to ensure that SimGrid itself has deterministic
  // behavior on all platforms, we always pick events with lower id's
  // to ensure we explore the unfolding deterministically.
  if (A.empty()) {
    const auto min_event = std::min_element(enC.begin(), enC.end(),
                                            [](const auto e1, const auto e2) { return e1->get_id() < e2->get_id(); });
    return const_cast<UnfoldingEvent*>(*min_event);
  } else {
    const auto intersection = A.make_intersection(enC);
    const auto min_event    = std::min_element(intersection.begin(), intersection.end(),
                                               [](const auto e1, const auto e2) { return e1->get_id() < e2->get_id(); });
    return const_cast<UnfoldingEvent*>(*min_event);
  }
}

void UdporChecker::clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D)
{
  // The "clean-up set" conceptually represents
  // those events which will no longer be considered
  // by UDPOR during its exploration. The concept is
  // introduced to avoid modification during iteration
  // over the current unfolding to determine who needs to
  // be removed. Since sets are unordered, it's quite possible
  // that e.g. two events `e` and `e'` such that `e < e'`
  // which are determined eligible for removal are removed
  // in the order `e` and then `e'`. Determining that `e'`
  // needs to be removed requires that its history be in
  // tact to e.g. compute the conflicts with the event.
  //
  // Thus, we compute the set and remove all of the events
  // at once in lieu of removing events while iterating over them.
  // We can hypothesize that processing the events in reverse
  // topological order would prevent any issues concerning
  // the order in which are processed
  EventSet clean_up_set;

  // Q_(C, D, U) = C u D u U (complicated expression)
  // See page 9 of "Unfolding-based Partial Order Reduction"

  // "C u D" portion
  const EventSet C_union_D = C.get_events().make_union(D);

  // "U (complicated expression)" portion
  const EventSet conflict_union = std::accumulate(
      C_union_D.begin(), C_union_D.end(), EventSet(), [&](const EventSet& acc, const UnfoldingEvent* e_prime) {
        return acc.make_union(unfolding.get_immediate_conflicts_of(e_prime));
      });

  const EventSet Q_CDU = C_union_D.make_union(conflict_union.get_local_config());

  XBT_DEBUG("Computed Q_CDU as '%s'", Q_CDU.to_string().c_str());

  // Move {e} \ Q_CDU from U to G
  if (not Q_CDU.contains(e)) {
    XBT_DEBUG("Moving %s from U to G...", e->to_string().c_str());
    clean_up_set.insert(e);
  }

  // foreach ê in #ⁱ_U(e)
  for (const auto* e_hat : this->unfolding.get_immediate_conflicts_of(e)) {
    // Move [ê] \ Q_CDU from U to G
    const EventSet to_remove = e_hat->get_local_config().subtracting(Q_CDU);
    XBT_DEBUG("Moving {%s} from U to G...", to_remove.to_string().c_str());
    clean_up_set.form_union(to_remove);
  }

  // TODO: We still perhaps need to
  // figure out how to deal with the fact that the previous
  // extension sets computed for past configurations
  // contain events that may be removed from `U`. Perhaps
  // it would be best to keep them around forever (they
  // are moved to `G` after all and can be discarded at will,
  // which means they may never have to be removed at all).
  //
  // Of course, the benefit of moving them into the set `G`
  // is that the computation for immediate conflicts becomes
  // more efficient (we have to search all of `U` for such conflicts,
  // and there would be no reason to search those events
  // that UDPOR has marked as no longer being important)
  // For now, there appear to be no "obvious" issues (although
  // UDPOR's behavior is often far from obvious...)
  this->unfolding.mark_finished(clean_up_set);
}

RecordTrace UdporChecker::get_record_trace()
{
  RecordTrace res;
  for (auto const& state : state_stack)
    res.push_back(state->get_transition_out().get());
  return res;
}

} // namespace simgrid::mc::udpor

namespace simgrid::mc {

Exploration* create_udpor_checker(const std::vector<char*>& args)
{
  return new simgrid::mc::udpor::UdporChecker(args);
}

} // namespace simgrid::mc
