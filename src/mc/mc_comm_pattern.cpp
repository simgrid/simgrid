/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/mc/mc_smx.hpp"

using simgrid::mc::remote;

static void MC_patterns_copy(std::vector<simgrid::mc::PatternCommunication*>& dest,
                             std::vector<simgrid::mc::PatternCommunication> const& source)
{
  dest.clear();
  for (simgrid::mc::PatternCommunication const& comm : source) {
    simgrid::mc::PatternCommunication* copy_comm = new simgrid::mc::PatternCommunication(comm.dup());
    dest.push_back(copy_comm);
  }
}

void MC_restore_communications_pattern(simgrid::mc::State* state)
{
  for (unsigned i = 0; i < initial_communications_pattern.size(); i++)
    initial_communications_pattern[i].index_comm = state->communication_indices_[i];

  for (unsigned i = 0; i < MC_smx_get_maxpid(); i++)
    MC_patterns_copy(incomplete_communications_pattern[i], state->incomplete_comm_pattern_[i]);
}

void MC_state_copy_incomplete_communications_pattern(simgrid::mc::State* state)
{
  state->incomplete_comm_pattern_.clear();
  for (unsigned i=0; i < MC_smx_get_maxpid(); i++) {
    std::vector<simgrid::mc::PatternCommunication> res;
    for (auto const& comm : incomplete_communications_pattern[i])
      res.push_back(comm->dup());
    state->incomplete_comm_pattern_.push_back(std::move(res));
  }
}

void MC_state_copy_index_communications_pattern(simgrid::mc::State* state)
{
  state->communication_indices_.clear();
  for (auto const& list_process_comm : initial_communications_pattern)
    state->communication_indices_.push_back(list_process_comm.index_comm);
}

void MC_handle_comm_pattern(e_mc_call_type_t call_type, smx_simcall_t req, int value, int backtracking)
{
  // HACK, do not rely on the Checker implementation outside of it
  simgrid::mc::CommunicationDeterminismChecker* checker =
    (simgrid::mc::CommunicationDeterminismChecker*) mc_model_checker->getChecker();

  switch(call_type) {
  case MC_CALL_TYPE_NONE:
    break;
  case MC_CALL_TYPE_SEND:
  case MC_CALL_TYPE_RECV:
    checker->get_comm_pattern(req, call_type, backtracking);
    break;
  case MC_CALL_TYPE_WAIT:
  case MC_CALL_TYPE_WAITANY:
    {
    simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr{nullptr};
    if (call_type == MC_CALL_TYPE_WAIT)
      comm_addr = remote(static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(req)));

    else {
      simgrid::kernel::activity::ActivityImpl* addr;
      addr      = mc_model_checker->process().read(remote(simcall_comm_waitany__getraw__comms(req) + value));
      comm_addr = remote(static_cast<simgrid::kernel::activity::CommImpl*>(addr));
      }
      checker->complete_comm_pattern(comm_addr, MC_smx_simcall_get_issuer(req)->get_pid(), backtracking);
    }
    break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }
}
