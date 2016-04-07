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

/* On every state, each process has an entry of the following type */
class ProcessState {
  /* Possible exploration status of a process in a state */
  enum class InterleavingType {
    /** We do not have to execute this process transitions */
    disabled = 0,
    /** We still have to execute (some of) this process transitions */
    interleave,
    /** We have already executed this process transitions */
    done,
  };

  /** Exploration control information */
  InterleavingType state = InterleavingType::disabled;
public:

  /** Number of times that the process was interleaved */
  // TODO, make this private
  unsigned int interleave_count = 0;

  bool isDisabled() const
  {
    return this->state == InterleavingType::disabled;
  }
  bool isDone() const
  {
    return this->state == InterleavingType::done;
  }
  bool isToInterleave() const
  {
    return this->state == InterleavingType::interleave;
  }
  void interleave()
  {
    this->state = InterleavingType::interleave;
    this->interleave_count = 0;
  }
  void setDone()
  {
    this->state = InterleavingType::done;
  }
};

/* An exploration state.
 *
 *  The `executed_state` is sometimes transformed into another `internal_req`.
 *  For example WAITANY is transformes into a WAIT and TESTANY into TEST.
 *  See `MC_state_get_request_for_process()`.
 */
struct XBT_PRIVATE State {

  /** Sequential state number (used for debugging) */
  int num = 0;

  /* Next transition to explore for this communication
   *
   * Some transitions are not deterministic such as:
   *
   * * waitany which can receive different messages;
   *
   * * random which can produce different values.
   *
   * This variable is used to keep track of which transition
   * should be explored next for a given simcall.
   */
  int req_num = 0;

  /** State's exploration status by process */
  std::vector<ProcessState> processStates;

  /** The simcall */
  s_smx_simcall_t executed_req;

  /* Internal translation of the simcall
   *
   * IMCALL_COMM_TESTANY is translated to a SIMCALL_COMM_TEST
   * and SIMCALL_COMM_WAITANY to a SIMCALL_COMM_WAIT.
   */
  s_smx_simcall_t internal_req;

  /* Can be used as a copy of the remote synchro object */
  s_smx_synchro_t internal_comm;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state;

  // For CommunicationDeterminismChecker
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern;
  std::vector<unsigned> communicationIndices;

  State();

  std::size_t interleaveSize() const;
  void interleave(smx_process_t process)
  {
    this->processStates[process->pid].interleave();
  }
};

XBT_PRIVATE void replay(std::list<std::unique_ptr<simgrid::mc::State>> const& stack);

}
}

XBT_PRIVATE simgrid::mc::State* MC_state_new(void);
XBT_PRIVATE smx_simcall_t MC_state_get_executed_request(simgrid::mc::State* state, int *value);
XBT_PRIVATE smx_simcall_t MC_state_get_internal_request(simgrid::mc::State* state);
XBT_PRIVATE smx_simcall_t MC_state_get_request(simgrid::mc::State* state, int *value);

#endif
