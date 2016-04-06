/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_STATE_H
#define SIMGRID_MC_STATE_H

#include <list>
#include <memory>

#include <xbt/base.h>
#include <xbt/dynar.h>

#include <simgrid_config.h>
#include "src/simix/smx_private.h"
#include "src/mc/mc_snapshot.h"

namespace simgrid {
namespace mc {

extern XBT_PRIVATE std::unique_ptr<s_mc_global_t> initial_global_state;

struct PatternCommunication {
  int num = 0;
  smx_synchro_t comm_addr;
  e_smx_comm_type_t type = SIMIX_COMM_SEND;
  unsigned long src_proc = 0;
  unsigned long dst_proc = 0;
  const char *src_host = nullptr;
  const char *dst_host = nullptr;
  std::string rdv;
  std::vector<char> data;
  int tag = 0;
  int index = 0;

  PatternCommunication()
  {
    std::memset(&comm_addr, 0, sizeof(comm_addr));
  }

  PatternCommunication dup() const
  {
    simgrid::mc::PatternCommunication res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type = this->type;
    // src_proc?
    // dst_proc?
    res.dst_proc = this->dst_proc;
    res.dst_host = this->dst_host;
    res.rdv = this->rdv;
    res.data = this->data;
    // tag?
    res.index = this->index;
    return res;
  }

};

/* Possible exploration status of a process in a state */
enum class ProcessInterleaveState {
  no_interleave=0, /* Do not interleave (do not execute) */
  interleave,      /* Interleave the process (one or more request) */
  more_interleave, /* Interleave twice the process (for mc_random simcall) */
  done             /* Already interleaved */
};

/* On every state, each process has an entry of the following type */
struct ProcessState {
  /** Exploration control information */
  ProcessInterleaveState state = ProcessInterleaveState::no_interleave;
  /** Number of times that the process was interleaved */
  unsigned int interleave_count;

  bool done() const
  {
    return this->state == ProcessInterleaveState::done;
  }
  bool interleave() const {
    return this->state == ProcessInterleaveState::interleave
      || this->state == ProcessInterleaveState::more_interleave;
  }
};

/* An exploration state.
 *
 *  The `executed_state` is sometimes transformed into another `internal_req`.
 *  For example WAITANY is transformes into a WAIT and TESTANY into TEST.
 *  See `MC_state_set_executed_request()`.
 */
struct XBT_PRIVATE State {
  /** State's exploration status by process */
  std::vector<ProcessState> processStates;
  s_smx_synchro_t internal_comm;        /* To be referenced by the internal_req */
  s_smx_simcall_t internal_req;         /* Internal translation of request */
  s_smx_simcall_t executed_req;         /* The executed request of the state */
  int req_num = 0;                      /* The request number (in the case of a
                                       multi-request like waitany ) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state = nullptr;      /* Snapshot of system state */
  int num = 0;
  int in_visited_states = 0;

  // comm determinism verification (xbt_dynar_t<xbt_dynar_t<simgrid::mc::PatternCommunication*>):
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern;

  // For communication determinism verification:
  std::vector<unsigned> communicationIndices;

  State();

  std::size_t interleaveSize() const;
};

XBT_PRIVATE void replay(std::list<std::unique_ptr<simgrid::mc::State>> const& stack);

}
}

XBT_PRIVATE simgrid::mc::State* MC_state_new(void);
XBT_PRIVATE void MC_state_interleave_process(simgrid::mc::State* state, smx_process_t process);
XBT_PRIVATE void MC_state_set_executed_request(simgrid::mc::State* state, smx_simcall_t req, int value);
XBT_PRIVATE smx_simcall_t MC_state_get_executed_request(simgrid::mc::State* state, int *value);
XBT_PRIVATE smx_simcall_t MC_state_get_internal_request(simgrid::mc::State* state);
XBT_PRIVATE smx_simcall_t MC_state_get_request(simgrid::mc::State* state, int *value);

#endif
