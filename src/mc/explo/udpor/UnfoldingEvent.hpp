/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_UNFOLDING_EVENT_HPP
#define SIMGRID_MC_UDPOR_UNFOLDING_EVENT_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"

#include <initializer_list>
#include <memory>
#include <string>

namespace simgrid::mc::udpor {

class UnfoldingEvent {
public:
  explicit UnfoldingEvent(std::initializer_list<UnfoldingEvent*> init_list);
  UnfoldingEvent(EventSet immediate_causes              = EventSet(),
                 std::shared_ptr<Transition> transition = std::make_unique<Transition>());

  UnfoldingEvent(const UnfoldingEvent&)            = default;
  UnfoldingEvent& operator=(UnfoldingEvent const&) = default;
  UnfoldingEvent(UnfoldingEvent&&)                 = default;

  EventSet get_history() const;
  bool in_history_of(const UnfoldingEvent* otherEvent) const;

  bool conflicts_with(const UnfoldingEvent* otherEvent) const;
  bool conflicts_with(const Configuration& config) const;
  bool immediately_conflicts_with(const UnfoldingEvent* otherEvt) const;

  const EventSet& get_immediate_causes() const { return this->immediate_causes; }
  Transition* get_transition() const { return this->associated_transition.get(); }

  bool operator==(const UnfoldingEvent&) const;

  /**
   * @brief The transition that UDPOR "attaches" to this
   * specific event for later use while computing e.g. extension
   * sets
   *
   * The transition points to that of a particular actor
   * in the state reached by the configuration C (recall
   * this is denoted `state(C)`) that excludes this event.
   * In other words, this transition was the "next" event
   * of the actor that executes it in `state(C)`.
   */
  std::shared_ptr<Transition> associated_transition;

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
};

} // namespace simgrid::mc::udpor
#endif