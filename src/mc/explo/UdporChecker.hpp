/* Copyright (c) 2007-2023. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CHECKER_HPP
#define SIMGRID_MC_UDPOR_CHECKER_HPP

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/udpor_global.hpp"

namespace simgrid::mc {

class XBT_PRIVATE UdporChecker : public Exploration {
public:
  explicit UdporChecker(const std::vector<char*>& args);

  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;

private:
  /**
   * The total number of events created whilst exploring the unfolding
   */
  uint32_t nb_events = 0;
  uint32_t nb_traces = 0;

  /**
   * The "relevant" portions of the unfolding that must be kept around to ensure that
   * UDPOR properly searches the state space
   *
   * The set `U` is a global variable which is maintained by UDPOR
   * to keep track of "just enough" information about the unfolding
   * to compute *alternatives* (see the paper for more details).
   *
   * @invariant: When a new event is created by UDPOR, it is inserted into
   * this set. All new events that are created by UDPOR have causes that
   * also exist in U and are valid for the duration of the search
   */
  EventSet U;
  EventSet G;

private:
  /**
   * @brief Explores the unfolding of the concurrent system
   * represented by the model checker instance
   * "model_checker"
   *
   * TODO: Explain what this means here
   */
  void explore(Configuration C, std::list<EventSet> maxEvtHistory, EventSet D, EventSet A, UnfoldingEvent* currentEvt,
               EventSet prev_exC);
};
} // namespace simgrid::mc

#endif
