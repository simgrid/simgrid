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
    auto* copy_comm = new simgrid::mc::PatternCommunication(comm.dup());
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

void MC_handle_comm_pattern(simgrid::mc::CallType call_type, smx_simcall_t req, int value, int backtracking)
{
  // HACK, do not rely on the Checker implementation outside of it
  auto* checker = static_cast<simgrid::mc::CommunicationDeterminismChecker*>(mc_model_checker->getChecker());

  using simgrid::mc::CallType;
  switch(call_type) {
    case CallType::NONE:
      break;
    case CallType::SEND:
    case CallType::RECV:
      checker->get_comm_pattern(req, call_type, backtracking);
      break;
    case CallType::WAIT:
    case CallType::WAITANY: {
      simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr{nullptr};
      if (call_type == CallType::WAIT)
        comm_addr = remote(simcall_comm_wait__getraw__comm(req));

      else {
        simgrid::kernel::activity::ActivityImpl* addr;
        addr = mc_model_checker->get_remote_simulation().read(remote(simcall_comm_waitany__getraw__comms(req) + value));
        comm_addr = remote(static_cast<simgrid::kernel::activity::CommImpl*>(addr));
      }
      checker->complete_comm_pattern(comm_addr, MC_smx_simcall_get_issuer(req)->get_pid(), backtracking);
    } break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }
}
