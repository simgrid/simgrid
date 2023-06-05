/* Copyright (c) 2007-2023. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CHECKER_HPP
#define SIMGRID_MC_UDPOR_CHECKER_HPP

#include "src/mc/api/State.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/mc_record.hpp"

#include <functional>
#include <list>
#include <optional>

namespace simgrid::mc::udpor {

/**
 * @brief Performs exploration of a concurrent system via the
 * UDPOR algorithm
 *
 * The `UdporChecker` implementation is based primarily off three papers,
 * herein referred to as [1], [2], and [3] respectively, as well as the
 * current implementation of `tiny_simgrid`:
 *
 * 1. "Unfolding-based Partial Order Reduction" by Rodriguez et al.
 * 2. "Quasi-Optimal Partial Order Reduction" by Nguyen et al.
 * 3. The Anh Pham's Thesis "Exploration efficace de l'espace ..."
 */
class XBT_PRIVATE UdporChecker : public Exploration {
public:
  explicit UdporChecker(const std::vector<char*>& args);

  void run() override;
  RecordTrace get_record_trace() override;
  std::unique_ptr<State> get_current_state() { return std::make_unique<State>(get_remote_app()); }

private:
  Unfolding unfolding = Unfolding();

  // The current sequence of states that the checker has
  // visited in order to reach the current configuration
  std::list<std::unique_ptr<State>> state_stack;

  /**
   * @brief Explores the unfolding of the concurrent system
   * represented by the ModelChecker instance "mcmodel_checker"
   *
   * This function performs the actual search following the
   * UDPOR algorithm according to [1].
   *
   * @param C the current configuration from which UDPOR will be used
   * to explore expansions of the concurrent system being modeled
   * @param D the set of events that should not be considered by UDPOR
   * while performing its searches, in order to avoid sleep-set blocked
   * executions. See [1] for more details
   * @param A the set of events to "guide" UDPOR in the correct direction
   * when it returns back to a node in the unfolding and must decide among
   * events to select from `ex(C)`. See [1] for more details
   *
   * TODO: Add the optimization where we can check if e == e_prior
   * to prevent repeated work when computing ex(C)
   */
  void explore(const Configuration& C, EventSet D, EventSet A, EventSet prev_exC);

  /**
   * @brief Identifies the next event from the unfolding of the concurrent system
   * that should next be explored as an extension of a configuration with
   * enabled events `enC`
   *
   * @param A The set of events `A` maintained by the UDPOR algorithm to help
   * determine how events should be selected. See the original paper [1] for more details
   *
   * @param enC The set `enC` of enabled events from the extension set `exC` used
   * by the UDPOR algorithm to select new events to search. See the original
   * paper [1] for more details
   */
  UnfoldingEvent* select_next_unfolding_event(const EventSet& A, const EventSet& enC);

  /**
   * @brief Computes the sets `ex(C)` and `en(C)` of the given configuration
   * `C` as an incremental computation from the the previous computation of `ex(C)`
   *
   * A central component to UDPOR is the computation of the set `ex(C)`. The
   * extension set `ex(C)` of a configuration `C` is defined as the set of events
   * outside of `C` whose full dependency chain is contained in `C` (see [1]
   * for more details).
   *
   * In general, computing `ex(C)` is very expensive. In paper [3], The Anh Pham
   * shows a method of incremental computation of the set `ex(C)` under the
   * conclusions afforded under the computation model in consideration, of which
   * SimGrid is apart, which allow for `ex(C)` to be computed much more efficiently.
   * Intuitively, the idea is to take advantage of the fact that you can avoid a lot
   * of repeated computation by exploiting the aforementioned properties (in [3]) in
   * what is akin to a dynamic programming optimization. See [3] for more details
   *
   * @param C the configuration based on which the two sets `ex(C)` and `en(C)` are
   * computed
   * @param stateC the state of the program after having executed C (viz. `state(C)`)
   * @param prev_exC the previous value of `ex(C)`, viz. that which was computed for
   * the configuration `C' := C - {e}`
   * @returns the extension set `ex(C)` of `C`
   */
  EventSet compute_exC(const Configuration& C, const State& stateC, const EventSet& prev_exC);
  EventSet compute_enC(const Configuration& C, const EventSet& exC) const;

  /**
   *
   */
  void move_to_stateCe(State* stateC, UnfoldingEvent* e);

  /**
   * @brief Creates a new snapshot of the state of the application
   * as it currently looks
   */
  std::unique_ptr<State> record_current_state();

  /**
   * @brief Move the application side into the state at the top of the
   * state stack provided
   *
   * As UDPOR performs its search, it moves the application-side along with
   * it so that the application is always providing the checker with
   * the correct information about what each actor is running (and whether
   * those actors are enabled) at the state reached by the configuration it
   * decides to search.
   *
   * When UDPOR decides to "backtrack" (e.g. after reaching a configuration
   * whose en(C) is empty), before it attempts to continue the search by taking
   * a different path from a configuration it visited in the past, it must ensure
   * that the application-side has moved back into `state(C)`.
   *
   * The search may have moved the application arbitrarily deep into its execution,
   * and the search may backtrack arbitrarily closer to the beginning of the execution.
   * The UDPOR implementation in SimGrid ensures that the stack is updated appropriately,
   * but the process must still be regenerated.
   */
  void restore_program_state_with_current_stack();

  /**
   * @brief Perform the functionality of the `Remove(e, C, D)` function in [1]
   */
  void clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D);
};
} // namespace simgrid::mc::udpor

#endif
