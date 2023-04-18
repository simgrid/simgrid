/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CONFIGURATION_HPP
#define SIMGRID_MC_UDPOR_CONFIGURATION_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace simgrid::mc::udpor {

class Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&)                 = default;

  explicit Configuration(const UnfoldingEvent* event);
  explicit Configuration(const EventSet& events);
  explicit Configuration(const History& history);
  explicit Configuration(std::initializer_list<const UnfoldingEvent*> events);

  auto begin() const { return this->events_.begin(); }
  auto end() const { return this->events_.end(); }
  auto cbegin() const { return this->events_.cbegin(); }
  auto cend() const { return this->events_.cend(); }

  bool contains(const UnfoldingEvent* e) const { return this->events_.contains(e); }
  bool contains(const EventSet& events) const { return events.is_subset_of(this->events_); }
  const EventSet& get_events() const { return this->events_; }
  const UnfoldingEvent* get_latest_event() const { return this->newest_event; }
  std::string to_string() const { return this->events_.to_string(); }

  /**
   * @brief Insert a new event into the configuration
   *
   * When the newly added event is inserted into the configuration,
   * an assertion is made to ensure that the configuration remains
   * valid, i.e. that the newly added event's dependencies are contained
   * within the configuration.
   *
   * @param e the event to add to the configuration. If the event is
   * already a part of the configuration, calling this method has no
   * effect.
   *
   * @throws an invalid argument exception is raised should the event
   * be missing one of its dependencies
   *
   * @note: UDPOR technically enforces the invariant that all newly-added events
   * will ensure that the configuration is valid. We perform extra checks to ensure
   * that UDPOR is implemented as expected. There is a performance penalty that
   * should be noted: checking for maximality requires ensuring that all events in the
   * configuration have their dependencies containes within the configuration, which
   * essentially means performing a BFS/DFS over the configuration using a History object.
   * However, since the slowest part of UDPOR involves enumerating all
   * subsets of maximal events and computing k-partial alternatives (the
   * latter definitively an NP-hard problem when optimal), Amdahl's law suggests
   * we shouldn't focus so much on this (let alone the additional benefit of the
   * assertions)
   */
  void add_event(const UnfoldingEvent* e);

  /**
   * @brief Whether or not the given event can be added to
   * this configuration while preserving that the configuration
   * is causally closed and conflict-free
   *
   * A configuration `C` is compatible with an event iff
   * the event can be added to `C` while preserving that
   * the configuration is causally closed and conflict-free.
   *
   * The method effectively answers the following question:
   *
   * "Is `C + {e}` a valid configuration?"
   */
  bool is_compatible_with(const UnfoldingEvent* e) const;

  /**
   * @brief Whether or not the events in the given history can be added to
   * this configuration while keeping the set of events causally closed
   * and conflict-free
   *
   * A configuration `C` is compatible with a history iff all
   * events of the history can be added to `C` while preserving
   * that the configuration is causally closed and conflict-free.
   *
   * The method effectively answers the following question:
   *
   * "Is `C + (U_i [e_i])` a valid configuration?"
   */
  bool is_compatible_with(const History& history) const;

  std::optional<Configuration> compute_alternative_to(const EventSet& D, const Unfolding& U) const;
  std::optional<Configuration> compute_k_partial_alternative_to(const EventSet& D, const Unfolding& U, size_t k) const;

  /**
   * @brief Orders the events of the configuration such that
   * "more recent" events (i.e. those that are farther down in
   * the event structure's dependency chain) come after those
   * that appeared "farther in the past"
   *
   * @returns a vector `V` with the following property:
   *
   * 1. Let i(e) := C -> I map events to their indices in `V`.
   * For every pair of events e, e' in C, if e < e' then i(e) < i(e')
   *
   * Intuitively, events that are closer to the "bottom" of the event
   * structure appear farther along in the list than those that appear
   * closer to the "top"
   */
  std::vector<const UnfoldingEvent*> get_topologically_sorted_events() const;

  /**
   * @brief Orders the events of the configuration such that
   * "more recent" events (i.e. those that are farther down in
   * the event structure's dependency chain) come before those
   * that appear "farther in the past"
   *
   * @note The events of the event structure are arranged such that
   * e < e' implies a directed edge from e to e'. However, it is
   * also useful to be able to traverse the *reverse* graph (for
   * example when computing the compatibility graph of a configuration),
   * hence the distinction between "reversed" and the method
   * "Configuration::get_topologically_sorted_events()"
   *
   * @returns a vector `V` with the following property:
   *
   * 1. Let i(e) := C -> I map events to their indices in `V`.
   * For every pair of events e, e' in C, if e < e' then i(e) > i(e')
   *
   * Intuitively, events that are closer to the "top" of the event
   * structure appear farther along in the list than those that appear
   * closer to the "bottom"
   */
  std::vector<const UnfoldingEvent*> get_topologically_sorted_events_of_reverse_graph() const;

  /**
   * @brief Computes the smallest set of events whose collective histories
   * capture all events of this configuration
   *
   * @invariant The set of all events in the collective histories
   * of the events returned by this method is equal to the set of events
   * in this configuration
   *
   * @returns the smallest set of events whose events generates
   * this configuration (denoted `config(E)`)
   */
  EventSet get_minimally_reproducible_events() const;

  /**
   * @brief Determines the event in the configuration whose associated
   * transition is the latest of the given actor
   *
   * @invariant: At most one event in the configuration will correspond
   * to `preEvt(C, a)` for any action `a`. This can be argued by contradiction.
   *
   * If there were more than one event (`e` and `e'`) in any configuration whose
   * associated transitions `a` were run by the same actor at the same step, then they
   * could not be causally related (`<`) since `a` could not be enabled in
   * both subconfigurations C' and C'' containing the hypothetical events
   * `e` and `e` + `e'`. Since they would not be contained in each other's histories,
   * they would be in conflict, which cannot happen between any pair of events
   * in a configuration. Thus `e` and `e'` cannot exist simultaneously
   */
  std::optional<const UnfoldingEvent*> get_latest_event_of(aid_t) const;
  /**
   * @brief Determines the most recent transition of the given actor
   * in this configuration, or `pre(a)` as denoted in the thesis of
   * The Anh Pham
   *
   * Conceptually, the execution of an interleaving of the transitions
   * (i.e. a topological ordering) of a configuration yields a unique
   * state `state(C)`. Since actions taken by the same actor are always
   * dependent with one another, any such interleaving always yields a
   * unique
   *
   * @returns the most recent transition of the given actor
   * in this configuration, or `std::nullopt` if there are no transitions
   * in this configuration run by the given actor
   */
  std::optional<const Transition*> get_latest_action_of(aid_t aid) const;
  std::optional<const UnfoldingEvent*> pre_event(aid_t aid) const { return get_latest_event_of(aid); }

private:
  /**
   * @brief The most recent event added to the configuration
   */
  const UnfoldingEvent* newest_event = nullptr;

  /**
   * @brief The events which make up this configuration
   *
   * @invariant For each event `e` in `events_`, the set of
   * dependencies of `e` is also contained in `events_`
   *
   * @invariant For each pair of events `e` and `e'` in
   * `events_`, `e` and `e'` are not in conflict
   */
  EventSet events_;

  /**
   * @brief Maps actors to the latest events which
   * are executed by the actor
   *
   * @invariant: The events that are contained in the map
   * are also contained in the set `events_`
   */
  std::unordered_map<aid_t, const UnfoldingEvent*> latest_event_mapping;
};

} // namespace simgrid::mc::udpor
#endif
