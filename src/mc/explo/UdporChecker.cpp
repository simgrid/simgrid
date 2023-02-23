/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/UdporChecker.hpp"
#include "src/mc/api/State.hpp"
#include <xbt/asserts.h>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to verification using UDPOR");

namespace simgrid::mc::udpor {

UdporChecker::UdporChecker(const std::vector<char*>& args) : Exploration(args)
{
  // Initialize the map
}

void UdporChecker::run()
{
  XBT_INFO("Starting a UDPOR exploration");
  // NOTE: `A`, `D`, and `C` are derived from the
  // original UDPOR paper [1], while `prev_exC` arises
  // from the incremental computation of ex(C) from [3]
  Configuration C_root;

  // TODO: Move computing the root configuration into a method on the Unfolding
  auto initial_state      = get_current_state();
  auto root_event         = std::make_unique<UnfoldingEvent>(EventSet(), std::make_shared<Transition>());
  auto* root_event_handle = root_event.get();
  unfolding.insert(std::move(root_event));
  C_root.add_event(root_event_handle);

  explore(std::move(C_root), EventSet(), EventSet(), std::move(initial_state), EventSet());

  XBT_INFO("UDPOR exploration terminated -- model checking completed");
}

void UdporChecker::explore(Configuration C, EventSet D, EventSet A, std::unique_ptr<State> stateC, EventSet prev_exC)
{
  // Perform the incremental computation of exC
  //
  // TODO: This method will have side effects on
  // the unfolding, but the naming of the method
  // suggests it is doesn't have side effects. We should
  // reconcile this in the future
  auto [exC, enC] = compute_extension(C, *stateC, prev_exC);

  // If enC is a subset of D, intuitively
  // there aren't any enabled transitions
  // which are "worth" exploring since their
  // exploration would lead to a so-called
  // "sleep-set blocked" trace.
  if (enC.is_subset_of(D)) {

    if (C.get_events().size() > 0) {

      // g_var::nb_traces++;

      // TODO: Log here correctly
      // XBT_DEBUG("\n Exploring executions: %d : \n", g_var::nb_traces);
      // ...
      // ...
    }

    // When `en(C)` is empty, intuitively this means that there
    // are no enabled transitions that can be executed from the
    // state reached by `C` (denoted `state(C)`), i.e. by some
    // execution of the transitions in C obeying the causality
    // relation. Here, then, we would be in a deadlock.
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

  // Move the application into stateCe and actually make note of that state
  move_to_stateCe(*stateC, *e);
  auto stateCe = record_current_state();

  // Ce := C + {e}
  Configuration Ce = C;
  Ce.add_event(e);

  A.remove(e);
  exC.remove(e);

  // Explore(C + {e}, D, A \ {e})
  explore(Ce, D, std::move(A), std::move(stateCe), std::move(exC));

  // D <-- D + {e}
  D.insert(e);

  // TODO: Determine a value of K to use or don't use it at all
  constexpr unsigned K = 10;
  auto J               = compute_partial_alternative(D, C, K);
  if (!J.empty()) {
    J.subtract(C.get_events());

    // Before searching the "right half", we need to make
    // sure the program actually reflects the fact
    // that we are searching again from `stateC` (the recursive
    // search moved the program into `stateCe`)
    restore_program_state_to(*stateC);

    // Explore(C, D + {e}, J \ C)
    explore(C, D, std::move(J), std::move(stateC), std::move(prev_exC));
  }

  // D <-- D - {e}
  D.remove(e);

  // Remove(e, C, D)
  clean_up_explore(e, C, D);
}

std::tuple<EventSet, EventSet> UdporChecker::compute_extension(const Configuration& C, const State& stateC,
                                                               const EventSet& prev_exC) const
{
  // See eqs. 5.7 of section 5.2 of [3]
  // C = C' + {e_cur}, i.e. C' = C - {e_cur}
  //
  // Then
  //
  // ex(C) = ex(C' + {e_cur}) = ex(C') / {e_cur} + U{<a, K> : K is maximal, a depends on all of K, a enabled at K }
  UnfoldingEvent* e_cur = C.get_latest_event();
  EventSet exC          = prev_exC;
  exC.remove(e_cur);

  for (const auto& [aid, actor_state] : stateC.get_actors_list()) {
    for (const auto& transition : actor_state.get_enabled_transitions()) {
      // First check for a specialized function that can compute the extension
      // set "quickly" based on its type. Otherwise, fall back to computing
      // the set "by hand"
      const auto specialized_extension_function = incremental_extension_functions.find(transition->type_);
      if (specialized_extension_function != incremental_extension_functions.end()) {
        exC.form_union((specialized_extension_function->second)(C, *transition));
      } else {
        exC.form_union(this->compute_extension_by_enumeration(C, *transition));
      }
    }
  }

  EventSet enC;

  return std::tuple<EventSet, EventSet>(exC, enC);
}

EventSet UdporChecker::compute_extension_by_enumeration(const Configuration& C, const Transition& action) const
{
  // TODO: Do something more interesting here
  return EventSet();
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
  state.execute_next(next_actor);
}

void UdporChecker::restore_program_state_to(const State& stateC)
{
  // TODO: Perform state regeneration in the same manner as is done
  // in the DFSChecker.cpp
}

std::unique_ptr<State> UdporChecker::record_current_state()
{
  auto next_state = this->get_current_state();

  // In UDPOR, we care about all enabled transitions in a given state
  next_state->mark_all_enabled_todo();

  return next_state;
}

UnfoldingEvent* UdporChecker::select_next_unfolding_event(const EventSet& A, const EventSet& enC)
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

EventSet UdporChecker::compute_partial_alternative(const EventSet& D, const Configuration& C, const unsigned k) const
{
  // TODO: Compute k-partial alternatives using [2]
  return EventSet();
}

void UdporChecker::clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D)
{
  // TODO: Perform clean up here
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
