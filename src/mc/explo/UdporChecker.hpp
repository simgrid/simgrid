/* Copyright (c) 2007-2023. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CHECKER_HPP
#define SIMGRID_MC_UDPOR_CHECKER_HPP

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/udpor_global.hpp"

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
  uint32_t nb_events = 0;
  uint32_t nb_traces = 0;

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

  /**
   * Maintains the mapping between handles referenced by events in
   * the current state of the unfolding
   */
  StateManager state_manager_;

private:
  /**
   * @brief Explores the unfolding of the concurrent system
   * represented by the ModelChecker instance "mcmodel_checker"
   *
   * This function performs the actual search following the
   * UDPOR algorithm according to [1].
   */
  void explore(Configuration C, EventSet D, EventSet A, std::list<EventSet> max_evt_history, UnfoldingEvent* cur_event,
               EventSet prev_exC);

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
   * @param cur_event the event where UDPOR currently "rests", viz. the event UDPOR
   * is now currently considering
   * @param prev_exC the previous value of `ex(C)`, viz. that which was computed for
   * the configuration `C' := C - {cur_event}`
   * @returns a tuple containing the pair of sets `ex(C)` and `en(C)` respectively
   */
  std::tuple<EventSet, EventSet> compute_extension(const Configuration& C, const std::list<EventSet>& max_evt_history,
                                                   const UnfoldingEvent& cur_event, const EventSet& prev_exC) const;

  /**
   *
   */
  void observe_unfolding_event(const UnfoldingEvent& event);
  State& get_state_referenced_by(const UnfoldingEvent& event);
  StateHandle record_newly_visited_state();

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
   *
   */
  EventSet compute_partial_alternative(const EventSet& D, const Configuration& C, const unsigned k) const;

  /**
   *
   */
  void clean_up_explore(const UnfoldingEvent* e, const Configuration& C, const EventSet& D);
};
} // namespace simgrid::mc::udpor

#endif
