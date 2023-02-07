/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/UdporChecker.hpp"
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to MC safety verification ");

namespace simgrid::mc::udpor {

UdporChecker::UdporChecker(const std::vector<char*>& args) : Exploration(args)
{
  /* Create initial data structures, if any ...*/
  XBT_INFO("Starting a UDPOR exploration");

  // TODO: Initialize state structures for the search
}

void UdporChecker::run()
{
  // NOTE: `A`, `D`, and `C` are derived from the
  // original UDPOR paper [1], while
  // `prev_exC` arises from the incremental computation
  // of ex(C) from the former paper described in [3]
  EventSet A, D;
  Configuration C;
  EventSet prev_exC;

  const auto initial_state    = get_current_state();
  const auto initial_state_id = state_manager_.record_state(std::move(initial_state));
  const auto root_event       = std::make_unique<UnfoldingEvent>(-1, "", EventSet(), initial_state_id);
  explore(std::move(C), std::move(A), std::move(D), {EventSet()}, root_event.get(), std::move(prev_exC));
}

void UdporChecker::explore(Configuration C, EventSet D, EventSet A, std::list<EventSet> max_evt_history,
                           UnfoldingEvent* cur_evt, EventSet prev_exC)
{
  // Perform the incremental computation of exC
  auto [exC, enC] = compute_extension(C, max_evt_history, *cur_evt, prev_exC);

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

  observe_unfolding_event(*cur_evt);
  const auto next_state_id = record_newly_visited_state();

  UnfoldingEvent* e = select_next_unfolding_event(A, enC);
  xbt_assert(e != nullptr, "UDPOR guarantees that an event will be chosen at each point in"
                           "the search, yet no events were actually chosen");
  e->set_state_id(next_state_id);

  // TODO: Clean up configuration code before moving into the actual
  // implementations of everything

  // Configuration is the same + event e
  // Ce = C + {e}
  Configuration Ce = C;
  Ce.add_event(e);

  max_evt_history.push_back(Ce.get_maxmimal_events());
  A.remove(e);
  exC.remove(e);

  // Explore(C + {e}, D, A \ {e})
  explore(Ce, D, std::move(A), max_evt_history, e, std::move(exC));

  // D <-- D + {e}
  D.insert(e);

  // TODO: Determine a value of K to use or don't use it at all
  constexpr unsigned K = 10;
  auto J               = compute_partial_alternative(D, C, K);
  if (!J.empty()) {
    J.subtract(C.get_events());
    max_evt_history.pop_back();

    // Explore(C, D + {e}, J \ C)
    explore(C, D, std::move(J), std::move(max_evt_history), cur_evt, std::move(prev_exC));
  }

  // D <-- D - {e}
  D.remove(e);

  // Remove(e, C, D)
  clean_up_explore(e, C, D);
}

std::tuple<EventSet, EventSet> UdporChecker::compute_extension(const Configuration& C,
                                                               const std::list<EventSet>& max_evt_history,
                                                               const UnfoldingEvent& cur_event,
                                                               const EventSet& prev_exC) const
{
  // exC.remove(cur_event);

  // TODO: Compute extend() as it exists in tiny_simgrid

  // exC.subtract(C);
  return std::tuple<EventSet, EventSet>();
}

State& UdporChecker::get_state_referenced_by(const UnfoldingEvent& event)
{
  const auto state_id      = event.get_state_id();
  const auto wrapped_state = this->state_manager_.get_state(state_id);
  xbt_assert(wrapped_state != std::nullopt,
             "\n\n****** FATAL ERROR ******\n"
             "To each UDPOR event corresponds a state,"
             "but state %lu does not exist\n"
             "******************\n\n",
             state_id);
  return wrapped_state.value().get();
}

void UdporChecker::observe_unfolding_event(const UnfoldingEvent& event)
{
  auto& state            = this->get_state_referenced_by(event);
  const aid_t next_actor = state.next_transition();
  xbt_assert(next_actor >= 0, "\n\n****** FATAL ERROR ******\n"
                              "In reaching this execution path, UDPOR ensures that at least one\n"
                              "one transition of the state of an visited event is enabled, yet no\n"
                              "state was actually enabled");
  state.execute_next(next_actor);
}

StateHandle UdporChecker::record_newly_visited_state()
{
  const auto next_state    = this->get_current_state();
  const auto next_state_id = this->state_manager_.record_state(std::move(next_state));

  // In UDPOR, we care about all enabled transitions in a given state
  next_state->mark_all_enabled_todo();
  return next_state_id;
}

UnfoldingEvent* UdporChecker::select_next_unfolding_event(const EventSet& A, const EventSet& enC)
{
  // TODO: Actually select an event here
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
