/* Copyright (c) 2016-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/VisitedState.hpp"
#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_comm_pattern.hpp"

#include <string>
#include <vector>

#ifndef SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP
#define SIMGRID_MC_COMMUNICATION_DETERMINISM_CHECKER_HPP

namespace simgrid {
namespace mc {

class XBT_PRIVATE CommunicationDeterminismChecker : public Checker {
public:
  explicit CommunicationDeterminismChecker(Session& session);
  ~CommunicationDeterminismChecker();
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;

private:
  void prepare();
  void real_run();
  void log_state() override;
  void deterministic_comm_pattern(int process, const simgrid::mc::PatternCommunication* comm, int backtracking);
  void restoreState();
public:
  // These are used by functions which should be moved in CommunicationDeterminismChecker:
  void get_comm_pattern(smx_simcall_t request, e_mc_call_type_t call_type, int backtracking);
  void complete_comm_pattern(simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr, unsigned int issuer,
                             int backtracking);

private:
  /** Stack representing the position in the exploration graph */
  std::list<std::unique_ptr<simgrid::mc::State>> stack_;
  simgrid::mc::VisitedStates visited_states_;
  unsigned long expanded_states_count_ = 0;

  bool initial_communications_pattern_done = false;
  bool recv_deterministic                  = true;
  bool send_deterministic                  = true;
  char *send_diff = nullptr;
  char *recv_diff = nullptr;
};

#endif

}
}
