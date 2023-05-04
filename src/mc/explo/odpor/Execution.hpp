/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_EXECUTION_HPP
#define SIMGRID_MC_ODPOR_EXECUTION_HPP

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/explo/odpor/odpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"

#include <list>
#include <optional>
#include <unordered_set>
#include <vector>

namespace simgrid::mc::odpor {

using ProcessSequence   = std::list<aid_t>;
using ExecutionSequence = std::list<const State*>;
using Hypothetical      = ExecutionSequence;

/**
 * @brief The occurrence of a transition in an execution
 */
class Event {
  std::pair<const Transition*, ClockVector> contents_;

public:
  Event()                        = default;
  Event(Event&&)                 = default;
  Event(const Event&)            = default;
  Event& operator=(const Event&) = default;

  explicit Event(std::pair<const Transition*, ClockVector> pair) : contents_(std::move(pair)) {}

  const Transition* get_transition() const { return std::get<0>(contents_); }
  const ClockVector& get_clock_vector() const { return std::get<1>(contents_); }
};

/**
 * @brief An ordered sequence of transitions which describe
 * the evolution of a process undergoing model checking
 *
 * An execution conceptually is just a string of actors
 * ids (e.g. "1.2.3.1.2.2.1.1"), where the `i`th occurrence
 * of actor id `j` corresponds to the `i`th action executed
 * by the actor with id `j` (viz. the `i`th step of actor `j`).
 * Executions can stand alone on their own or can extend
 * the execution of other sequences
 *
 * Executions are conceived based on the following papers:
 * 1. "Source Sets: A Foundation for Optimal Dynamic Partial Order Reduction"
 * by Abdulla et al.
 *
 * In addition to representing an actual steps taken,
 * an execution keeps track of the "happens-before"
 * relation among the transitions in the execution
 * by following the procedure outlined in the
 * original DPOR paper with clock vectors
 *
 * @note: For more nuanced happens-before relations, clock
 * vectors may not always suffice. Clock vectors work
 * well with transition-based dependencies like that used in
 * SimGrid; but to have a more refined independence relation,
 * an event-based dependency approach is needed. See the section 2
 * in the ODPOR paper [1] concerning event-based dependencies and
 * how the happens-before relation can be refined in a
 * computation model much like that of SimGrid. In fact, the same issue
 * arrises with UDPOR with context-sensitive dependencies:
 * the two concepts are analogous if not identical
 */
class Execution {
private:
  /**
   * @brief The actual steps that are taken by the process
   * during exploration, relative to the
   */
  std::vector<Event> contents_;

  Execution(std::vector<Event>&& contents) : contents_(std::move(contents)) {}

public:
  using Handle      = decltype(contents_)::const_iterator;
  using EventHandle = uint32_t;

  Execution()                            = default;
  Execution(const Execution&)            = default;
  Execution& operator=(Execution const&) = default;
  Execution(Execution&&)                 = default;
  Execution(ExecutionSequence&& seq);
  Execution(const ExecutionSequence& seq);

  size_t size() const { return this->contents_.size(); }
  bool empty() const { return this->contents_.empty(); }
  auto begin() const { return this->contents_.begin(); }
  auto end() const { return this->contents_.end(); }

  std::optional<aid_t> get_first_ssdpor_initial_from(EventHandle e, std::unordered_set<aid_t> disqualified) const;
  std::unordered_set<aid_t> get_ssdpor_initials_from(EventHandle e, std::unordered_set<aid_t> disqualified) const;

  // std::unordered_set<aid_t> get_initials_after(const Hypothetical& w) const;
  // std::unordered_set<aid_t> get_weak_initials_after(const Hypothetical& w) const;

  // std::unordered_set<aid_t> get_initials_after(const Hypothetical& w) const;
  // std::unordered_set<aid_t> get_weak_initials_after(const Hypothetical& w) const;

  // bool is_initial(aid_t p, const Hypothetical& w) const;
  // bool is_weak_initial(aid_t p, const Hypothetical& w) const;

  const Event& get_event_with_handle(EventHandle handle) const { return contents_[handle]; }
  aid_t get_actor_with_handle(EventHandle handle) const { return get_event_with_handle(handle).get_transition()->aid_; }

  /**
   * @brief Returns a set of IDs of events which are in
   * "immediate conflict" (according to the definition given
   * in the ODPOR paper) with one another
   */
  std::unordered_set<EventHandle> get_racing_events_of(EventHandle) const;

  /**
   * @brief Returns a handle to the newest event of the execution,
   * if such an event exists
   */
  std::optional<EventHandle> get_latest_event_handle() const
  {
    return contents_.empty() ? std::nullopt : std::optional<EventHandle>{static_cast<EventHandle>(size() - 1)};
  }

  Execution get_prefix_up_to(EventHandle) const;

  /**
   * @brief Whether the event represented by `e1`
   * "happens-before" the event represented by
   * `e2` in the context of this execution
   *
   * In the terminology of the ODPOR paper,
   * this function computes
   *
   * `e1 --->_E e2`
   *
   * where `E` is this execution
   *
   * @note: The happens-before relation computed by this
   * execution is "coarse" in the sense that context-sensitive
   * independence is not exploited. To include such context-sensitive
   * dependencies requires a new method of keeping track of
   * the happens-before procedure, which is nontrivial...
   */
  bool happens_before(EventHandle e1, EventHandle e2) const;

  /**
   * @brief Removes the last event of the execution,
   * if such an event exists
   *
   * @note: When you remove events from an execution, any views
   * of the execution referring to those removed events
   * become invalidated
   */
  void pop_latest();

  /**
   * @brief Extends the execution by one more step
   *
   * Intutively, pushing a transition `t` onto execution `E`
   * is equivalent to making the execution become (using the
   * notation of [1]) `E.proc(t)` where `proc(t)` is the
   * actor which executed transition `t`.
   */
  void push_transition(const Transition*);
};

} // namespace simgrid::mc::odpor
#endif
