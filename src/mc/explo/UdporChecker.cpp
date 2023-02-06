/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/UdporChecker.hpp"
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to MC safety verification ");

namespace simgrid::mc {

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

  /**
   * Maintains the mapping between handles referenced by events in
   * the current state of the unfolding
   */
  StateManager state_manager_;

  const auto initial_state    = std::make_unique<State>(get_remote_app());
  const auto initial_state_id = state_manager_.record_state(std::move(initial_state));
  const auto root_event       = std::make_unique<UnfoldingEvent>(-1, "", EventSet(), initial_state_id);
  explore(std::move(C), {EventSet()}, std::move(D), std::move(A), root_event.get(), std::move(prev_exC));
}

void UdporChecker::explore(Configuration C, std::list<EventSet> max_evt_history, EventSet D, EventSet A,
                           UnfoldingEvent* cur_evt, EventSet prev_exC)
{
  // Perform the incremental computation of exC
  auto [exC, enC] = this->extend(C, max_evt_history, *cur_evt, prev_exC);

  // If enC is a subset of D, intuitively
  // there aren't any enabled transitions
  // which are "worth" exploring since their
  // exploration would lead to a so-called
  // "sleep-set blocked" trace.
  if (enC.is_subset_of(D)) {

    // Log traces
    if (C.events_.size() > 0) {

      // g_var::nb_traces++;

      // TODO: Log here correctly
      // XBT_DEBUG("\n Exploring executions: %d : \n", g_var::nb_traces);
      // ...
      // ...
    }

    // When `en(C)` is empty, intuitively this means that there
    // are no enabled transitions that can be executed from the
    // state reached by `C` (denoted `state(C)`) (i.e. by some
    // execution of the transitions in C obeying the causality
    // relation). Hence, it is at this point we should check for a deadlock
    if (enC.empty()) {
      get_remote_app().check_deadlock();
    }

    return;
  }

  UnfoldingEvent* e = this->select_next_unfolding_event(A, enC);
  if (e == nullptr) {
    XBT_ERROR("\n\n****** CRITICAL ERROR ****** \n"
              "UDPOR guarantees that an event will be chosen here, yet no events were actually chosen...\n"
              "******************");
    DIE_IMPOSSIBLE;
  }

  // TODO: Add verbose logging about which event is being explored

  // TODO: Execute the transition associated with the event
  // and map the new state

  // const auto cur_state_id = cur_evt->get_state_id();
  // auto curEv_StateId = currentEvt->get_state_id();
  // auto nextState_id  = App::app_side_->execute_transition(curEv_StateId, e->get_transition_tag());
  // e->set_state_id(nextState_id);

  // Configuration is the same + event e
  // C1 = C + {e}
  Configuration C1 = C;
  C1.events_.insert(e);
  C1.updateMaxEvent(e);

  max_evt_history.push_back(C1.maxEvent);

  // A <-- A \ {e}, ex(C) <-- ex(C) \ {e}
  A.remove(e);
  exC.remove(e);

  // Explore(C + {e}, D, A \ {e})
  this->explore(C1, max_evt_history, D, A, e, exC);

  // D <-- D + {e}
  D.insert(e);

  // TODO: Determine a value of K to use or don't use it at all
  constexpr unsigned K = 10;
  auto J               = this->compute_partial_alternative(D, C, K);
  if (!J.empty()) {
    J.subtract(C.events_);
    max_evt_history.pop_back();
    explore(C, max_evt_history, D, J, cur_evt, prev_exC);
  }

  // D <-- D - {e}
  D.remove(e);
  this->clean_up_explore(e, C, D);
}

std::tuple<EventSet, EventSet> UdporChecker::extend(const Configuration& C, const std::list<EventSet>& max_evt_history,
                                                    const UnfoldingEvent& cur_event, const EventSet& prev_exC) const
{
  // exC.remove(cur_event);

  // TODO: Compute extend() as it exists in tiny_simgrid

  // exC.subtract(C);
  return std::tuple<EventSet, EventSet>();
}

UnfoldingEvent* UdporChecker::select_next_unfolding_event(const EventSet& A, const EventSet& enC)
{
  // TODO: Actually select an event here
  return nullptr;
}

EventSet UdporChecker::compute_partial_alternative(const EventSet& D, const EventSet& C, const unsigned k) const
{
  // TODO: Compute k-partial alternatives using [2]
  return EventSet();
}

void UdporChecker::clean_up_explore(const UnfoldingEvent* e, const EventSet& C, const EventSet& D)
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

Exploration* create_udpor_checker(const std::vector<char*>& args)
{
  return new UdporChecker(args);
}

} // namespace simgrid::mc
