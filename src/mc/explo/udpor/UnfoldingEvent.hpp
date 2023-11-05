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
  explicit UnfoldingEvent(std::initializer_list<const UnfoldingEvent*> init_list);
  UnfoldingEvent(EventSet immediate_causes              = EventSet(),
                 std::shared_ptr<Transition> transition = std::make_unique<Transition>());

  UnfoldingEvent(const UnfoldingEvent&)            = default;
  UnfoldingEvent& operator=(UnfoldingEvent const&) = default;
  UnfoldingEvent(UnfoldingEvent&&)                 = default;

  EventSet get_history() const;
  EventSet get_local_config() const;
  bool in_history_of(const UnfoldingEvent* other) const;

  /**
   * @brief Whether or not the given event is a decendant
   * of or an ancestor of the given event
   */
  bool related_to(const UnfoldingEvent* other) const;

  /// @brief Whether or not this event is in conflict with
  /// the given one (i.e. whether `this # other`)
  bool conflicts_with(const UnfoldingEvent* other) const;

  /// @brief Whether or not this event is in conflict with
  /// any event in the given set
  bool conflicts_with_any(const EventSet& events) const;

  /// @brief Computes "this #ⁱ other"
  bool immediately_conflicts_with(const UnfoldingEvent* other) const;
  bool is_dependent_with(const Transition*) const;
  bool is_dependent_with(const UnfoldingEvent* other) const;

  unsigned get_id() const { return this->id; }
  aid_t get_actor() const { return get_transition()->aid_; }
  const EventSet& get_immediate_causes() const { return this->immediate_causes; }
  Transition* get_transition() const { return this->associated_transition.get(); }

  void set_transition(std::shared_ptr<Transition> t) { this->associated_transition = std::move(t); }

  std::string to_string() const;

  bool operator==(const UnfoldingEvent&) const;
  bool operator!=(const UnfoldingEvent& other) const { return not(*this == other); }

private:
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

  /**
   * @brief An identifier which is used to sort events
   * deterministically
   */
  unsigned long id = 0;
};

} // namespace simgrid::mc::udpor
#endif