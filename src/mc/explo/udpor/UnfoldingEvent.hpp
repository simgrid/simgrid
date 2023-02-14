/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_UNFOLDING_EVENT_HPP
#define SIMGRID_MC_UDPOR_UNFOLDING_EVENT_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

namespace simgrid::mc::udpor {

class UnfoldingEvent {
public:
  UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes,
                 StateHandle sid);
  UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes);
  UnfoldingEvent(const UnfoldingEvent&)            = default;
  UnfoldingEvent& operator=(UnfoldingEvent const&) = default;
  UnfoldingEvent(UnfoldingEvent&&)                 = default;

  EventSet get_history() const;
  bool in_history_of(const UnfoldingEvent* otherEvent) const;

  bool conflicts_with(const UnfoldingEvent* otherEvent) const;
  bool conflicts_with(const Configuration& config) const;
  bool immediately_conflicts_with(const UnfoldingEvent* otherEvt) const;

  inline StateHandle get_state_id() const { return state_id; }
  inline void set_state_id(StateHandle sid) { state_id = sid; }

  bool operator==(const UnfoldingEvent&) const
  {
    // TODO: Implement semantic equality
    return false;
  };

private:
  int id = -1;

  /**
   * @brief The "immediate" causes of this event.
   *
   * An event `e` is an immediate cause of an event `e'` if
   *
   * 1. e < e'
   * 2. There is no event `e''` in E such that
   * `e < e''` and `e'' < e'`
   *
   * Intuitively, an immediate cause "happened right before"
   * this event was executed. It is sufficient to store
   * only the immediate causes of any event `e`, as any indirect
   * causes of that event would either be an indirect cause
   * or an immediate cause of the immediate causes of `e`, and
   * so on.
   */
  EventSet immediate_causes;
  StateHandle state_id = 0ul;
};

} // namespace simgrid::mc::udpor
#endif