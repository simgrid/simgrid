/* Copyright (c) 2007-2023. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CHECKER_HPP
#define SIMGRID_MC_UDPOR_CHECKER_HPP

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/mc_record.hpp"

#include <functional>
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
 * 2. Quasi-Optimal Partial Order Reduction by Nguyen et al.
 * 3. The Anh Pham's Thesis "Exploration efficace de l'espace ..."
 */
class XBT_PRIVATE UdporChecker : public Exploration {
public:
  explicit UdporChecker(const std::vector<char*>& args);

  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;

  inline std::unique_ptr<State> get_current_state() { return std::make_unique<State>(get_remote_app()); }

private:
  /**
   * The total number of events created whilst exploring the unfolding
   */
  /* FIXME: private fields are not used
    uint32_t nb_events = 0;
    uint32_t nb_traces = 0;
  */

  /**
   * @brief The "relevant" portions of the unfolding that must be kept around to ensure that
   * UDPOR properly searches the state space
   *
   * The set `U` is a global variable which is maintained by UDPOR
   * to keep track of "just enough" information about the unfolding
   * to compute *alternatives* (see the paper for more details).
   *
   * @invariant: When a new event is created by UDPOR, it is inserted into
   * this set. All new events that are created by UDPOR have causes that
   * also exist in U and are valid for the duration of the search.
   *
   * If an event is discarded instead of moved from set `U` to set `G`,
   * the event and its contents will be discarded.
   */
  EventSet U;

  /**
   * @brief The "irrelevant" portions of the unfolding that do not need to be kept
   * around to ensure that UDPOR functions correctly
   *
   * The set `G` is another global variable maintained by the UDPOR algorithm which
   * is used to keep track of all events which used to be important to UDPOR
   */
  EventSet G;

  /// @brief UDPOR's current "view" of the program it is exploring
  Unfolding unfolding = Unfolding();

  using ExtensionFunction = std::function<EventSet(const Configuration&, const Transition&)>;

  /**
   * @brief A collection of specialized functions which can incrementally
   * compute the extension of a configuration based on the action taken
   */
  std::unordered_map<Transition::Type, ExtensionFunction> incremental_extension_functions =
      std::unordered_map<Transition::Type, ExtensionFunction>();

private:
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
   * @param stateC the state of the program after having executed `C`,
   * viz. `state(C)`  using the notation of [1]
   *
   * TODO: Add the optimization where we can check if e == e_prior
   * to prevent repeated work when computing ex(C)
   */
  void explore(const Configuration& C, EventSet D, EventSet A, std::unique_ptr<State> stateC, EventSet prev_exC);

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
   * what is effectively a dynamic programming optimization. See [3] for more details
   *
   * @param C the configuration based on which the two sets `ex(C)` and `en(C)` are
   * computed
   * @param stateC the state of the program after having executed C (viz. `state(C)`)
   * @param prev_exC the previous value of `ex(C)`, viz. that which was computed for
   * the configuration `C' := C - {e}`
   * @returns a tuple containing the pair of sets `ex(C)` and `en(C)` respectively
   */
  std::tuple<EventSet, EventSet> compute_extension(const Configuration& C, const State& stateC,
                                                   const EventSet& prev_exC) const;

  EventSet compute_extension_by_enumeration(const Configuration& C, const Transition& action) const;

  /**
   *
   */
  EventSet compute_partial_alternative(const EventSet& D, const Configuration& C, const unsigned k) const;

  /**
   *
   */
  void move_to_stateCe(State& stateC, const UnfoldingEvent& e);

  /**
   * @brief Creates a new snapshot of the state of the progam undergoing
   * model checking
   *
   * @returns the handle used to uniquely identify this state later in the
   * exploration of the unfolding. You provide this handle to an event in the
   * unfolding to regenerate past states
   */
  std::unique_ptr<State> record_current_state();

  /**
   *
   */
  void restore_program_state_to(const State& stateC);

  /**
   *
   */
  void clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D);
};
} // namespace simgrid::mc::udpor

#endif
