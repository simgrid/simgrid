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

/**
 * @brief The occurrence of a transition in an execution
 *
 * An execution is set of *events*, where each element represents
 * the occurrence or execution of the `i`th step of a particular
 * actor `j`
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
 * by following the procedure outlined in section 4 of the
 * original DPOR paper with clock vectors.
 * As new transitions are added to the execution, clock vectors are
 * computed as appropriate and associated with the corresponding position
 * in the execution. This allows us to determine “happens-before” in
 * constant-time between points in the execution (called events
 * [which is unfortunately the same name used in UDPOR for a slightly
 * different concept]), albeit for an up-front cost of traversing the
 * execution stack. The happens-before relation is important in many
 * places in SDPOR and ODPOR.
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

  size_t size() const { return this->contents_.size(); }
  bool empty() const { return this->contents_.empty(); }
  auto begin() const { return this->contents_.begin(); }
  auto end() const { return this->contents_.end(); }

  /**
   * @brief Computes the "core" portion the SDPOR algorithm,
   * viz. the intersection of the backtracking set and the
   * set of initials with respect to the *last* event added
   * to the execution
   *
   * The "core" portion of the SDPOR algorithm is found on
   * lines 6-9 of the pseudocode:
   *
   * 6 | let E' := pre(E, e)
   * 7 | let v :=  notdep(e, E).p
   * 8 | if I_[E'](v) ∩ backtrack(E') = empty then
   * 9 |    --> add some q in I_[E'](v) to backtrack(E')
   *
   * This method computes all of the lines simultaneously,
   * returning some actor `q` if it passes line 8 and exists.
   * The event `e` and the set `backtrack(E')` are the provided
   * arguments to the method.
   *
   * @param e the event with respect to which to determine
   * whether a backtrack point needs to be added for the
   * prefix corresponding to the execution prior to `e`
   *
   * @param backtrack_set The set of actors which should
   * not be considered for selection as an SDPOR initial.
   * While this set need not necessarily correspond to the
   * backtrack set `backtrack(E')`, doing so provides what
   * is expected for SDPOR
   *
   * See the SDPOR algorithm pseudocode in [1] for more
   * details for the context of the function.
   *
   * @invariant: This method assumes that events `e` and
   * `e' := get_latest_event_handle()` are in a *reversible* race
   * as is explicitly the case in SDPOR
   *
   * @returns an actor not contained in `disqualified` which
   * can serve as an initial to reverse the race between `e`
   * and `e'`
   */
  std::optional<aid_t> get_first_sdpor_initial_from(EventHandle e, std::unordered_set<aid_t> backtrack_set) const;

  /**
   * @brief For a given sequence of actors `v` and a sequence of transitions `w`,
   * computes the sequence, if any, that should be inserted as a child a WakeupTree for
   * this execution
   */
  std::optional<ProcessSequence> get_shortest_odpor_sq_subset_insertion(const ProcessSequence& v,
                                                                        const ExecutionSequence& w) const;

  /**
   * @brief Determines the event associated with
   * the given handle `handle`
   */
  const Event& get_event_with_handle(EventHandle handle) const { return contents_[handle]; }

  /**
   * @brief Determines the actor associated with
   * the given event handle `handle`
   */
  aid_t get_actor_with_handle(EventHandle handle) const { return get_event_with_handle(handle).get_transition()->aid_; }

  /**
   * @brief Returns a set of events which are in
   * "immediate conflict" (according to the definition given
   * in the ODPOR paper) with the given event
   *
   * Two events `e` and `e'` in an execution `E` are said to
   * race iff
   *
   * 1. `proc(e) != proc(e')`; that is, the events correspond to
   * the execution of different actors
   * 2. `e -->_E e'` and there is no `e''` in `E` such that
   *  `e -->_E e''` and `e'' -->_E e'`; that is, the two events
   * "happen-before" one another in `E` and no other event in
   * `E` "happens-between" `e` and `e'`
   *
   * @param handle the event with respect to which races are
   * computed
   * @returns a set of event handles from which race with `handle`
   */
  std::unordered_set<EventHandle> get_racing_events_of(EventHandle handle) const;

  /**
   * @brief Returns a handle to the newest event of the execution,
   * if such an event exists
   */
  std::optional<EventHandle> get_latest_event_handle() const
  {
    return contents_.empty() ? std::nullopt : std::optional<EventHandle>{static_cast<EventHandle>(size() - 1)};
  }

  /**
   * @brief Computes `pre(e, E)` as described in ODPOR [1]
   *
   * The execution `pre(e, E)` for an event `e` in an
   * execution `E` is the contiguous prefix of events
   * `E' <= E` up to by excluding the event `e` itself.
   * The prefix intuitively represents the "history" of
   * causes that permitted event `e` to exist (roughly
   * speaking)
   */
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
