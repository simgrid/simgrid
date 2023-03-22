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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to verification using UDPOR");

namespace simgrid::mc::udpor {

UdporChecker::UdporChecker(const std::vector<char*>& args) : Exploration(args, true) {}

void UdporChecker::run()
{
  XBT_INFO("Starting a UDPOR exploration");

  auto state_stack = std::list<std::unique_ptr<State>>();
  state_stack.push_back(get_current_state());

  explore(Configuration(), EventSet(), EventSet(), state_stack, EventSet());
  XBT_INFO("UDPOR exploration terminated -- model checking completed");
}

void UdporChecker::explore(const Configuration& C, EventSet D, EventSet A,
                           std::list<std::unique_ptr<State>>& state_stack, EventSet prev_exC)
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

  const UnfoldingEvent* e = select_next_unfolding_event(A, enC);
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
  move_to_stateCe(stateC, *e);
  state_stack.push_back(record_current_state());

  explore(Ce, D, std::move(A), state_stack, std::move(exC));

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
    restore_program_state_with_sequence(state_stack);

    // Explore(C, D + {e}, J \ C)
    auto J_minus_C = J.value().get_events().subtracting(C.get_events());
    explore(C, D, std::move(J_minus_C), state_stack, std::move(prev_exC));
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

EventSet UdporChecker::compute_exC_by_enumeration(const Configuration& C, const std::shared_ptr<Transition> action)
{
  // Here we're computing the following:
  //
  // U{<a, K> : K is maximal, `a` depends on all of K, `a` enabled at config(K) }
  //
  // where `a` is the `action` given to us. Note that `a` is presumed to be enabled
  EventSet incremental_exC;

  for (auto begin =
           maximal_subsets_iterator(C, {[&](const UnfoldingEvent* e) { return e->is_dependent_with(action.get()); }});
       begin != maximal_subsets_iterator(); ++begin) {
    const EventSet& maximal_subset = *begin;

    // Determining if `a` is enabled here might not be possible while looking at `a` opaquely
    // We leave the implementation as-is to ensure that any addition would be simple
    // if it were ever added
    const bool enabled_at_config_k = false;

    if (enabled_at_config_k) {
      auto event        = std::make_unique<UnfoldingEvent>(maximal_subset, action);
      const auto handle = unfolding.insert(std::move(event));
      incremental_exC.insert(handle);
    }
  }
  return incremental_exC;
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

void UdporChecker::move_to_stateCe(State& state, const UnfoldingEvent& e)
{
  const aid_t next_actor = e.get_transition()->aid_;

  // TODO: Add the trace if possible for reporting a bug
  xbt_assert(next_actor >= 0, "\n\n****** INVARIANT VIOLATION ******\n"
                              "In reaching this execution path, UDPOR ensures that at least one\n"
                              "one transition of the state of an visited event is enabled, yet no\n"
                              "state was actually enabled. Please report this as a bug.\n"
                              "*********************************\n\n");
  state.execute_next(next_actor, get_remote_app());
}

void UdporChecker::restore_program_state_with_sequence(const std::list<std::unique_ptr<State>>& state_stack)
{
  get_remote_app().restore_initial_state();

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (const std::unique_ptr<State>& state : state_stack) {
    if (state == stack_.back()) /* If we are arrived on the target state, don't replay the outgoing transition */
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

const UnfoldingEvent* UdporChecker::select_next_unfolding_event(const EventSet& A, const EventSet& enC)
{
  if (!enC.empty()) {
    return *(enC.begin());
  }

  for (const auto& event : A) {
    if (enC.contains(event)) {
      return event;
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
  return res;
}

std::vector<std::string> UdporChecker::get_textual_trace()
{
  // TODO: Topologically sort the events of the latest configuration
  // and iterate through that topological sorting
  std::vector<std::string> trace;
  return trace;
}

} // namespace simgrid::mc::udpor

namespace simgrid::mc {

Exploration* create_udpor_checker(const std::vector<char*>& args)
{
  return new simgrid::mc::udpor::UdporChecker(args);
}

} // namespace simgrid::mc
