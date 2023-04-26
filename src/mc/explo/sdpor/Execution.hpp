/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_ODPOR_EXECUTION_HPP
#define SIMGRID_MC_ODPOR_EXECUTION_HPP

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/transition/Transition.hpp"

#include <unordered_set>
#include <vector>

namespace simgrid::mc::odpor {

/**
 * @brief An ordered sequence of actions
 */
class ExecutionSequence : public std::list<const Transition*> {};
using Hypothetical = ExecutionSequence;

/**
 * @brief The occurrence of a transition in an execution
 */
class Event {
  std::pair<const Transition*, ClockVector> contents_;

public:
  Event()                         = default;
  Event(Event&&)                  = default;
  Event(const Event&)             = default;
  Event& operator=(const Event&)  = default;
  Event& operator=(const Event&&) = default;

  explicit Event(std::pair<const Transition*, ClockVector> pair) : contents_(std::move(pair)) {}

  const Transition* get_transition() const { return contents_.get<0>(); }
  const ClockVector& get_clock_vector() const { return contents_.get<1>(); }
}

/**
 * @brief An ordered sequence of transitions which describe
 * the evolution of a process undergoing model checking
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
   * during exploration.
   */
  std::list<Event> contents_;

public:
  Execution()                            = default;
  Execution(const Execution&)            = default;
  Execution& operator=(Execution const&) = default;
  Execution(Execution&&)                 = default;
  explicit Execution(ExecutionSequence&& seq);
  explicit Execution(const ExecutionSequence& seq);

  std::unordered_set<aid_t> get_initials_after(const Hypothetical& w) const;
  std::unordered_set<aid_t> get_weak_initials_after(const Hypothetical& w) const;

  bool is_initial(aid_t p, const Hypothetical& w) const;
  bool is_weak_initial(aid_t p, const Hypothetical& w) const;

  void take_action(const Transition*);
};

} // namespace simgrid::mc::odpor
#endif
