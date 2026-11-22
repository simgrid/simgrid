/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* RaceDetector: the data race detector of the model checker.
 *
 * # Where the memory accesses come from
 *
 * When the verified application is compiled with the smemory LLVM pass, each load and store calls the smemory runtime,
 * which records the access in the MemoryAccessTrace of the current actor. When an actor reaches its next simcall, its
 * trace is sent to the checker along with the corresponding transition, and the actor's trace is emptied. So each
 * Transition carries the accesses that its actor did since its previous transition: we call this a segment of the
 * actor. Transition::get_memory_trace_or_null() returns nullptr when the segment did no access.
 *
 * # Which accesses race
 *
 * Two accesses race if they come from different actors, touch the same byte, at least one of them is a write, and
 * neither happens before the other. The happens-before relation is given by the clock vectors of the Execution: the
 * event of index j happens before the event e iff j <= CV_e[actor of j]. These clock vectors come from the dependency
 * relation used by the reduction: two dependent transitions (e.g., an unlock and the next lock of the same mutex) are
 * ordered, and this order is transitive.
 *
 * A segment happens after everything that its actor knew when it started, that is after everything that happens
 * before the previous event p of that actor (or before the event that created the actor). The event that ends the
 * segment does not order the accesses of the segment: they were done before it. For example, the accesses that an
 * actor does right before locking a mutex are not protected by this mutex.
 *
 * # How the races are searched
 *
 * The search itself is done by SlabTrackState (see SlabTrackState.hpp), a variant of the FastTrack algorithm that works
 * on intervals of memory. This class translates the events of the Execution into the clock convention of SlabTrack,
 * where the clock 0 means "no access":
 *  - The epoch of the accesses carried by the event of index i, done by actor A, is (A, i + 1).
 *  - The vector clock of that segment is i + 1 for A, and CV_p[u] + 1 for each other actor u that p knows, or 0 if
 *    p knows nothing about u (or if there is no p).
 * With this convention, an access carried by the event j of actor u races with the segment of A iff j + 1 is greater
 * than CV_p[u] + 1, that is iff j does not happen before p. Each Event stores the handle of p
 * (Event::get_predecessor()), which is needed for every event that carries accesses, including the old events
 * re-applied by a rebuild (see below).
 *
 * The clock of each epoch of a McDataRace is thus the rank (starting from 1) of the event that carries the access in
 * the execution, which is also its rank in the textual trace shown to the user.
 *
 * # Following the exploration
 *
 * The explorers (DFS, BeFS, and each thread of the parallel explorer) own a RaceDetector, and call check() after each
 * transition that they execute and add to their Execution. The replay mode does the same. The search of the critical
 * transition does not search for races. A RaceDetector is not thread-safe: each thread must have its own.
 *
 * check() applies the events that are new since the previous call. To find them, it remembers the transitions that it
 * saw (known_), and compares them to the execution from its end: the execution only changes at its end, and each
 * transition belongs to a single state of the exploration tree, so the first match gives the prefix that did not
 * change. Holding these transitions also prevents their addresses from being reused by new transitions.
 *
 * The shadow memory only describes the current branch of the exploration. When an event whose accesses were applied
 * is no longer in the execution (the explorer backtracked over it), the shadow is reset and rebuilt from scratch with
 * the accesses of the whole execution. This costs the same order of time as the replay of the prefix that the
 * application does when the explorer backtracks, but each step is much cheaper (no inter-process communication), and
 * it needs no memory per state of the exploration. The events that carry no access are never applied: removing them
 * does not trigger any rebuild, so an exploration without memory instrumentation never rebuilds anything.
 *
 * check() stops at the first race and throws McDataRace. The shadow memory is then left in an inconsistent state, so
 * the next call to check() rebuilds it.
 */

#ifndef SIMGRID_MC_ODPOR_RACE_DETECTOR_HPP
#define SIMGRID_MC_ODPOR_RACE_DETECTOR_HPP

#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/smemory/SlabTrackState.hpp"

#include <optional>
#include <vector>

namespace simgrid::mc::odpor {

/** Detects the data races of the executions explored by the model checker (see the top of this file). */
class RaceDetector {
  smemory::SlabTrackState shadow_;
  // The transitions of the execution prefix that check() saw. Holding them prevents their addresses from being reused.
  std::vector<TransitionPtr> known_;
  // Index of the last event whose accesses are in the shadow, if any
  std::optional<Execution::EventHandle> last_applied_;

  unsigned long rebuild_count_ = 0; // For statistics
  unsigned long applied_count_ = 0; // For statistics

  void apply(const Execution& exec, Execution::EventHandle e);

public:
  /** Applies the accesses of the events that are new since the previous call, rebuilding the shadow if needed.
   *  Throws McDataRace on the first race. */
  void check(const Execution& exec);

  unsigned long get_rebuild_count() const { return rebuild_count_; }
  unsigned long get_applied_count() const { return applied_count_; }
};

} // namespace simgrid::mc::odpor
#endif
