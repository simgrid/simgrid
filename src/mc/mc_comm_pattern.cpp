/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstring>

#include "xbt/dynar.h"
#include "xbt/sysdep.h"
#include <xbt/dynar.hpp>

#include "src/mc/checker/CommunicationDeterminismChecker.hpp"
#include "src/mc/mc_comm_pattern.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/mc_xbt.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_pattern, mc,
                                "Logging specific to MC communication patterns");

static void MC_patterns_copy(xbt_dynar_t dest,
  std::vector<simgrid::mc::PatternCommunication> const& source)
{
  xbt_dynar_reset(dest);
  for (simgrid::mc::PatternCommunication const& comm : source) {
    simgrid::mc::PatternCommunication* copy_comm = new simgrid::mc::PatternCommunication(comm.dup());
    xbt_dynar_push(dest, &copy_comm);
  }
}

void MC_restore_communications_pattern(simgrid::mc::State* state)
{
  simgrid::mc::PatternCommunicationList* list_process_comm;
  unsigned int cursor;

  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm)
    list_process_comm->index_comm = state->communicationIndices[cursor];

  for (unsigned i = 0; i < MC_smx_get_maxpid(); i++)
    MC_patterns_copy(
      xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t),
      state->incomplete_comm_pattern[i]
    );
}

void MC_state_copy_incomplete_communications_pattern(simgrid::mc::State* state)
{
  state->incomplete_comm_pattern.clear();
  for (unsigned i=0; i < MC_smx_get_maxpid(); i++) {
    xbt_dynar_t patterns = xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t);
    std::vector<simgrid::mc::PatternCommunication> res;
    simgrid::mc::PatternCommunication* comm;
    unsigned int cursor;
    xbt_dynar_foreach(patterns, cursor, comm)
      res.push_back(comm->dup());
    state->incomplete_comm_pattern.push_back(std::move(res));
  }
}

void MC_state_copy_index_communications_pattern(simgrid::mc::State* state)
{
  state->communicationIndices.clear();
  simgrid::mc::PatternCommunicationList* list_process_comm;
  unsigned int cursor;
  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm)
    state->communicationIndices.push_back(list_process_comm->index_comm);
}

void MC_handle_comm_pattern(
  e_mc_call_type_t call_type, smx_simcall_t req,
  int value, xbt_dynar_t pattern, int backtracking)
{
  // HACK, do not rely on the Checker implementation outside of it
  simgrid::mc::CommunicationDeterminismChecker* checker =
    (simgrid::mc::CommunicationDeterminismChecker*) mc_model_checker->getChecker();

  switch(call_type) {
  case MC_CALL_TYPE_NONE:
    break;
  case MC_CALL_TYPE_SEND:
  case MC_CALL_TYPE_RECV:
    checker->get_comm_pattern(pattern, req, call_type, backtracking);
    break;
  case MC_CALL_TYPE_WAIT:
  case MC_CALL_TYPE_WAITANY:
    {
    simgrid::mc::RemotePtr<simgrid::kernel::activity::CommImpl> comm_addr{nullptr};
    if (call_type == MC_CALL_TYPE_WAIT)
      comm_addr = remote(static_cast<simgrid::kernel::activity::CommImpl*>(simcall_comm_wait__getraw__comm(req)));

    else {
      simgrid::kernel::activity::CommImpl* addr;
      // comm_addr = REMOTE(xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value, smx_synchro_t)):
      simgrid::mc::read_element(mc_model_checker->process(), &addr, remote(simcall_comm_waitany__get__comms(req)),
                                value, sizeof(comm_addr));
      comm_addr = remote(addr);
      }
      checker->complete_comm_pattern(pattern, comm_addr,
        MC_smx_simcall_get_issuer(req)->pid, backtracking);
    }
    break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }

}
