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
#include "src/mc/mc_record.h"
#include "src/mc/Transition.hpp"

namespace simgrid {
namespace mc {

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

/* A node in the exploration graph (kind-of)
 */
struct XBT_PRIVATE State {

  /** Sequential state number (used for debugging) */
  int num = 0;

  /** State's exploration status by process */
  std::vector<ProcessState> processStates;

  Transition transition;

  /** The simcall which was executed */
  s_smx_simcall_t executed_req;

  /* Internal translation of the simcall
   *
   * SIMCALL_COMM_TESTANY is translated to a SIMCALL_COMM_TEST
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
  Transition getTransition() const;
};

}
}

XBT_PRIVATE simgrid::mc::State* MC_state_new(unsigned long state_number);
XBT_PRIVATE smx_simcall_t MC_state_get_request(simgrid::mc::State* state);

#endif
