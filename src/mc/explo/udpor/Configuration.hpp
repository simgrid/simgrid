/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CONFIGURATION_HPP
#define SIMGRID_MC_UDPOR_CONFIGURATION_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

namespace simgrid::mc::udpor {

using StateHandle = uint64_t;

class Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&)                 = default;

  inline auto begin() const { return this->events_.begin(); }
  inline auto end() const { return this->events_.end(); }
  inline const EventSet& get_events() const { return this->events_; }
  inline const EventSet& get_maximal_events() const { return this->max_events_; }

  void add_event(UnfoldingEvent*);
  UnfoldingEvent* get_latest_event() const;

private:
  /**
   * @brief The most recent event added to the configuration
   */
  UnfoldingEvent* newest_event = nullptr;
  EventSet events_;

  /**
   * The <-maxmimal events of the configuration. These are
   * dynamically adjusted as events are added to the configuration
   *
   * @invariant: Each event that is part of this set is
   *
   * 1. a <-maxmimal event of the configuration, in the sense that
   * there is no event in the configuration that is "greater" than it.
   * In UDPOR terminology, each event does not "cause" another event
   *
   * 2. contained in the set of events `events_` which comprise
   * the configuration
   */
  EventSet max_events_;

private:
  void recompute_maxmimal_events(UnfoldingEvent* new_event);
};

class UnfoldingEvent {
public:
  UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes,
                 StateHandle sid);
  UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& immediate_causes);
  UnfoldingEvent(const UnfoldingEvent&)            = default;
  UnfoldingEvent& operator=(UnfoldingEvent const&) = default;
  UnfoldingEvent(UnfoldingEvent&&)                 = default;

  EventSet get_history() const;
  bool in_history(const UnfoldingEvent* otherEvent) const;

  bool conflicts_with(const UnfoldingEvent* otherEvent) const;
  bool conflicts_with(const Configuration& config) const;
  bool immediately_conflicts_with(const UnfoldingEvent* otherEvt) const;

  bool operator==(const UnfoldingEvent&) const { return false; };

  inline StateHandle get_state_id() const { return state_id; }
  inline void set_state_id(StateHandle sid) { state_id = sid; }

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

class StateManager {
private:
  using Handle = StateHandle;

  Handle current_handle_ = 1ul;
  std::unordered_map<Handle, std::unique_ptr<State>> state_map_;

public:
  Handle record_state(std::unique_ptr<State>);
  std::optional<std::reference_wrapper<State>> get_state(Handle);
};

} // namespace simgrid::mc::udpor
#endif
