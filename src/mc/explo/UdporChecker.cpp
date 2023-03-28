/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/UdporChecker.hpp"
#include "src/mc/api/State.hpp"
#include "src/mc/explo/udpor/Comb.hpp"
#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>
#include <xbt/string.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to verification using UDPOR");

namespace simgrid::mc::udpor {

UdporChecker::UdporChecker(const std::vector<char*>& args) : Exploration(args, true) {}

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

  // If enC is a subset of D, intuitively
  // there aren't any enabled transitions
  // which are "worth" exploring since their
  // exploration would lead to a so-called
  // "sleep-set blocked" trace.
  if (enC.is_subset_of(D)) {
    if (not C.get_events().empty()) {
      // Report information...
    }

    // When `en(C)` is empty, intuitively this means that there
    // are no enabled transitions that can be executed from the
    // state reached by `C` (denoted `state(C)`), i.e. by some
    // execution of the transitions in C obeying the causality
    // relation. Here, then, we may be in a deadlock (the other
    // possibility is that we've finished running everything, and
    // we wouldn't be in deadlock then)
    if (enC.empty()) {
      get_remote_app().check_deadlock();
    }

    return;
  }

  // TODO: Add verbose logging about which event is being explored

  UnfoldingEvent* e = select_next_unfolding_event(A, enC);
  xbt_assert(e != nullptr, "\n\n****** INVARIANT VIOLATION ******\n"
                           "UDPOR guarantees that an event will be chosen at each point in\n"
                           "the search, yet no events were actually chosen\n"
                           "*********************************\n\n");
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

  //  Prepare to move the application back one state.
  // We need only remove the state from the stack here: if we perform
  // another `Explore()` after computing an alternative, at that
  // point we'll actually create a fresh RemoteProcess
  state_stack.pop_back();

  // D <-- D + {e}
  D.insert(e);

  constexpr unsigned K = 10;
  if (auto J = C.compute_k_partial_alternative_to(D, this->unfolding, K); J.has_value()) {
    // Before searching the "right half", we need to make
    // sure the program actually reflects the fact
    // that we are searching again from `state(C)`. While the
    // stack of states is properly adjusted to represent
    // `state(C)` all together, the RemoteApp is currently sitting
    // at some *future* state with resepct to `state(C)` since the
    // recursive calls have moved it there.
    restore_program_state_with_current_stack();

    // Explore(C, D + {e}, J \ C)
    auto J_minus_C = J.value().get_events().subtracting(C.get_events());
    explore(C, D, std::move(J_minus_C), std::move(prev_exC));
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

  for (const auto& [aid, actor_state] : stateC.get_actors_list()) {
    for (const auto& transition : actor_state.get_enabled_transitions()) {
      EventSet extension = ExtensionSetCalculator::partially_extend(C, &unfolding, transition);
      exC.form_union(extension);
    }
  }
  return exC;
}

EventSet UdporChecker::compute_enC(const Configuration& C, const EventSet& exC) const
{
  EventSet enC;
  for (const auto e : exC) {
    if (not e->conflicts_with(C)) {
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
  get_remote_app().restore_initial_state();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (const std::unique_ptr<State>& state : state_stack) {
    if (state == state_stack.back()) /* If we are arrived on the target state, don't replay the outgoing transition */
      break;
    state->get_transition()->replay(get_remote_app());
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

  if (A.empty()) {
    return const_cast<UnfoldingEvent*>(*(enC.begin()));
  }

  for (const auto& event : A) {
    if (enC.contains(event)) {
      return const_cast<UnfoldingEvent*>(event);
    }
  }
  return nullptr;
}

void UdporChecker::clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D)
{
  const EventSet C_union_D              = C.get_events().make_union(D);
  const EventSet es_immediate_conflicts = this->unfolding.get_immediate_conflicts_of(e);
  const EventSet Q_CDU                  = C_union_D.make_union(es_immediate_conflicts.get_local_config());

  // Move {e} \ Q_CDU from U to G
  if (Q_CDU.contains(e)) {
    this->unfolding.remove(e);
  }

  // foreach ê in #ⁱ_U(e)
  for (const auto* e_hat : es_immediate_conflicts) {
    // Move [ê] \ Q_CDU from U to G
    const EventSet to_remove = e_hat->get_history().subtracting(Q_CDU);
    this->unfolding.remove(to_remove);
  }
}

RecordTrace UdporChecker::get_record_trace()
{
  RecordTrace res;
  for (auto const& state : state_stack)
    res.push_back(state->get_transition());
  return res;
}

std::vector<std::string> UdporChecker::get_textual_trace()
{
  std::vector<std::string> trace;
  for (auto const& state : state_stack) {
    const auto* t = state->get_transition();
    trace.push_back(xbt::string_printf("%ld: %s", t->aid_, t->to_string().c_str()));
  }
  return trace;
}

} // namespace simgrid::mc::udpor

namespace simgrid::mc {

Exploration* create_udpor_checker(const std::vector<char*>& args)
{
  return new simgrid::mc::udpor::UdporChecker(args);
}

} // namespace simgrid::mc
