/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_STATE_H
#define MC_STATE_H

#include <simgrid_config.h>
#include "../simix/smx_private.h"
#include "mc_snapshot.h"

SG_BEGIN_DECL()

extern mc_global_t initial_global_state;

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

/* An exploration state.
 *
 *  The `executed_state` is sometimes transformed into another `internal_req`.
 *  For example WAITANY is transformes into a WAIT and TESTANY into TEST.
 *  See `MC_state_set_executed_request()`.
 */
typedef struct mc_state {
  unsigned long max_pid;            /* Maximum pid at state's creation time */
  mc_procstate_t proc_status;       /* State's exploration status by process */
  s_smx_synchro_t internal_comm;     /* To be referenced by the internal_req */
  s_smx_simcall_t internal_req;         /* Internal translation of request */
  s_smx_simcall_t executed_req;         /* The executed request of the state */
  int req_num;                      /* The request number (in the case of a
                                       multi-request like waitany ) */
  mc_snapshot_t system_state;      /* Snapshot of system state */
  int num;
} s_mc_state_t, *mc_state_t;

mc_state_t MC_state_new(void);
void MC_state_delete(mc_state_t state);
void MC_state_interleave_process(mc_state_t state, smx_process_t process);
unsigned int MC_state_interleave_size(mc_state_t state);
int MC_state_process_is_done(mc_state_t state, smx_process_t process);
void MC_state_set_executed_request(mc_state_t state, smx_simcall_t req, int value);
smx_simcall_t MC_state_get_executed_request(mc_state_t state, int *value);
smx_simcall_t MC_state_get_internal_request(mc_state_t state);
smx_simcall_t MC_state_get_request(mc_state_t state, int *value);
void MC_state_remove_interleave_process(mc_state_t state, smx_process_t process);

SG_END_DECL()

#endif
