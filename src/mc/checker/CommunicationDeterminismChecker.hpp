/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

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
  explicit CommunicationDeterminismChecker();
  ~CommunicationDeterminismChecker() override;
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;

private:
  void prepare();
  void real_run();
  void log_state() override;
  void deterministic_comm_pattern(aid_t process, const PatternCommunication* comm, int backtracking);
  void restoreState();
  void handle_comm_pattern(simgrid::mc::CallType call_type, smx_simcall_t req, int value, int backtracking);

public:
  // These are used by functions which should be moved in CommunicationDeterminismChecker:
  void get_comm_pattern(smx_simcall_t request, CallType call_type, int backtracking);
  void complete_comm_pattern(RemotePtr<kernel::activity::CommImpl> const& comm_addr, aid_t issuer, int backtracking);

private:
  /** Stack representing the position in the exploration graph */
  std::list<std::unique_ptr<State>> stack_;
  VisitedStates visited_states_;
  unsigned long expanded_states_count_ = 0;

  bool initial_communications_pattern_done = false;
  bool recv_deterministic                  = true;
  bool send_deterministic                  = true;
  char *send_diff = nullptr;
  char *recv_diff = nullptr;
};
} // namespace mc
} // namespace simgrid

#endif

