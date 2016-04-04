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

/* Possible exploration status of a process in a state */
typedef enum {
  MC_NOT_INTERLEAVE=0,      /* Do not interleave (do not execute) */
  MC_INTERLEAVE,            /* Interleave the process (one or more request) */
  MC_MORE_INTERLEAVE,       /* Interleave twice the process (for mc_random simcall) */
  MC_DONE                   /* Already interleaved */
} e_mc_process_state_t;

/* On every state, each process has an entry of the following type */
typedef struct mc_procstate{
  e_mc_process_state_t state;       /* Exploration control information */
  unsigned int interleave_count;    /* Number of times that the process was
                                       interleaved */
} s_mc_procstate_t, *mc_procstate_t;

namespace simgrid {
namespace mc {

extern XBT_PRIVATE std::unique_ptr<s_mc_global_t> initial_global_state;

/* An exploration state.
 *
 *  The `executed_state` is sometimes transformed into another `internal_req`.
 *  For example WAITANY is transformes into a WAIT and TESTANY into TEST.
 *  See `MC_state_set_executed_request()`.
 */
struct XBT_PRIVATE State {
  unsigned long max_pid = 0;            /* Maximum pid at state's creation time */
  mc_procstate_t proc_status = 0;       /* State's exploration status by process */
  s_smx_synchro_t internal_comm;     /* To be referenced by the internal_req */
  s_smx_simcall_t internal_req;         /* Internal translation of request */
  s_smx_simcall_t executed_req;         /* The executed request of the state */
  int req_num = 0;                      /* The request number (in the case of a
                                       multi-request like waitany ) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state = nullptr;      /* Snapshot of system state */
  int num = 0;
  int in_visited_states = 0;
  // comm determinism verification (xbt_dynar_t<xbt_dynar_t<mc_comm_pattern_t>):
  xbt_dynar_t incomplete_comm_pattern = nullptr;
  xbt_dynar_t index_comm = nullptr; // comm determinism verification

  State();
  ~State();
};

XBT_PRIVATE void replay(std::list<std::unique_ptr<simgrid::mc::State>> const& stack);

}
}

XBT_PRIVATE simgrid::mc::State* MC_state_new(void);
XBT_PRIVATE void MC_state_delete(simgrid::mc::State* state, int free_snapshot);
XBT_PRIVATE void MC_state_interleave_process(simgrid::mc::State* state, smx_process_t process);
XBT_PRIVATE unsigned int MC_state_interleave_size(simgrid::mc::State* state);
XBT_PRIVATE int MC_state_process_is_done(simgrid::mc::State* state, smx_process_t process);
XBT_PRIVATE void MC_state_set_executed_request(simgrid::mc::State* state, smx_simcall_t req, int value);
XBT_PRIVATE smx_simcall_t MC_state_get_executed_request(simgrid::mc::State* state, int *value);
XBT_PRIVATE smx_simcall_t MC_state_get_internal_request(simgrid::mc::State* state);
XBT_PRIVATE smx_simcall_t MC_state_get_request(simgrid::mc::State* state, int *value);
XBT_PRIVATE void MC_state_remove_interleave_process(simgrid::mc::State* state, smx_process_t process);

#endif
