/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <assert.h>

#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/simix/smx_private.h"
#include "src/mc/mc_state.h"
#include "src/mc/mc_request.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_xbt.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_state, mc,
                                "Logging specific to MC (state)");

extern "C" {

/**
 * \brief Creates a state data structure used by the exploration algorithm
 */
mc_state_t MC_state_new()
{
  mc_state_t state = xbt_new0(s_mc_state_t, 1);

  state->max_pid = MC_smx_get_maxpid();
  state->proc_status = xbt_new0(s_mc_procstate_t, state->max_pid);
  state->system_state = nullptr;
  state->num = ++mc_stats->expanded_states;
  state->in_visited_states = 0;
  state->incomplete_comm_pattern = nullptr;
  /* Stateful model checking */
  if((_sg_mc_checkpoint > 0 && (mc_stats->expanded_states % _sg_mc_checkpoint == 0)) ||  _sg_mc_termination){
    state->system_state = simgrid::mc::take_snapshot(state->num);
    if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
      MC_state_copy_incomplete_communications_pattern(state);
      MC_state_copy_index_communications_pattern(state);
    }
  }
  return state;
}

/**
 * \brief Deletes a state data structure
 * \param trans The state to be deleted
 */
void MC_state_delete(mc_state_t state, int free_snapshot){
  if (state->system_state && free_snapshot)
    delete state->system_state;
  if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
    xbt_free(state->index_comm);
    xbt_free(state->incomplete_comm_pattern);
  }
  xbt_free(state->proc_status);
  xbt_free(state);
}

void MC_state_interleave_process(mc_state_t state, smx_process_t process)
{
  assert(state);
  state->proc_status[process->pid].state = MC_INTERLEAVE;
  state->proc_status[process->pid].interleave_count = 0;
}

void MC_state_remove_interleave_process(mc_state_t state, smx_process_t process)
{
  if (state->proc_status[process->pid].state == MC_INTERLEAVE)
    state->proc_status[process->pid].state = MC_DONE;
}

unsigned int MC_state_interleave_size(mc_state_t state)
{
  unsigned int i, size = 0;
  for (i = 0; i < state->max_pid; i++)
    if ((state->proc_status[i].state == MC_INTERLEAVE)
        || (state->proc_status[i].state == MC_MORE_INTERLEAVE))
      size++;
  return size;
}

int MC_state_process_is_done(mc_state_t state, smx_process_t process)
{
  return state->proc_status[process->pid].state == MC_DONE ? TRUE : FALSE;
}

void MC_state_set_executed_request(mc_state_t state, smx_simcall_t req,
                                   int value)
{
  state->executed_req = *req;
  state->req_num = value;

  /* The waitany and testany request are transformed into a wait or test request over the
   * corresponding communication action so it can be treated later by the dependence
   * function. */
  switch (req->call) {
  case SIMCALL_COMM_WAITANY: {
    state->internal_req.call = SIMCALL_COMM_WAIT;
    state->internal_req.issuer = req->issuer;
    smx_synchro_t remote_comm;
    read_element(mc_model_checker->process(),
      &remote_comm, remote(simcall_comm_waitany__get__comms(req)),
      value, sizeof(remote_comm));
    mc_model_checker->process().read(&state->internal_comm, remote(remote_comm));
    simcall_comm_wait__set__comm(&state->internal_req, &state->internal_comm);
    simcall_comm_wait__set__timeout(&state->internal_req, 0);
    break;
  }

  case SIMCALL_COMM_TESTANY:
    state->internal_req.call = SIMCALL_COMM_TEST;
    state->internal_req.issuer = req->issuer;

    if (value > 0) {
      smx_synchro_t remote_comm;
      read_element(mc_model_checker->process(),
        &remote_comm, remote(simcall_comm_testany__get__comms(req)),
        value, sizeof(remote_comm));
      mc_model_checker->process().read(&state->internal_comm, remote(remote_comm));
    }

    simcall_comm_test__set__comm(&state->internal_req, &state->internal_comm);
    simcall_comm_test__set__result(&state->internal_req, value);
    break;

  case SIMCALL_COMM_WAIT:
    state->internal_req = *req;
    mc_model_checker->process().read_bytes(&state->internal_comm ,
      sizeof(state->internal_comm), remote(simcall_comm_wait__get__comm(req)));
    simcall_comm_wait__set__comm(&state->executed_req, &state->internal_comm);
    simcall_comm_wait__set__comm(&state->internal_req, &state->internal_comm);
    break;

  case SIMCALL_COMM_TEST:
    state->internal_req = *req;
    mc_model_checker->process().read_bytes(&state->internal_comm,
      sizeof(state->internal_comm), remote(simcall_comm_test__get__comm(req)));
    simcall_comm_test__set__comm(&state->executed_req, &state->internal_comm);
    simcall_comm_test__set__comm(&state->internal_req, &state->internal_comm);
    break;

  case SIMCALL_MC_RANDOM: {
    state->internal_req = *req;
    int random_max = simcall_mc_random__get__max(req);
    if (value != random_max)
      for (auto& p : mc_model_checker->process().simix_processes()) {
        mc_procstate_t procstate = &state->proc_status[p.copy.pid];
        const smx_process_t issuer = MC_smx_simcall_get_issuer(req);
        if (p.copy.pid == issuer->pid) {
          procstate->state = MC_MORE_INTERLEAVE;
          break;
        }
      }
    break;
  }

  default:
    state->internal_req = *req;
    break;
  }
}

smx_simcall_t MC_state_get_executed_request(mc_state_t state, int *value)
{
  *value = state->req_num;
  return &state->executed_req;
}

smx_simcall_t MC_state_get_internal_request(mc_state_t state)
{
  return &state->internal_req;
}

static inline smx_simcall_t MC_state_get_request_for_process(
  mc_state_t state, int*value, smx_process_t process)
{
  mc_procstate_t procstate = &state->proc_status[process->pid];

  if (procstate->state != MC_INTERLEAVE
      && procstate->state != MC_MORE_INTERLEAVE)
      return nullptr;
  if (!simgrid::mc::process_is_enabled(process))
    return nullptr;

  switch (process->simcall.call) {

      case SIMCALL_COMM_WAITANY:
        *value = -1;
        while (procstate->interleave_count <
              read_length(mc_model_checker->process(),
                remote(simcall_comm_waitany__get__comms(&process->simcall)))) {
          if (simgrid::mc::request_is_enabled_by_idx(&process->simcall,
              procstate->interleave_count++)) {
            *value = procstate->interleave_count - 1;
            break;
          }
        }

        if (procstate->interleave_count >=
            simgrid::mc::read_length(mc_model_checker->process(),
              simgrid::mc::remote(simcall_comm_waitany__get__comms(&process->simcall))))
          procstate->state = MC_DONE;

        if (*value != -1)
          return &process->simcall;

        break;

      case SIMCALL_COMM_TESTANY: {
        unsigned start_count = procstate->interleave_count;
        *value = -1;
        while (procstate->interleave_count <
                read_length(mc_model_checker->process(),
                  remote(simcall_comm_testany__get__comms(&process->simcall))))
          if (simgrid::mc::request_is_enabled_by_idx(&process->simcall,
              procstate->interleave_count++)) {
            *value = procstate->interleave_count - 1;
            break;
          }

        if (procstate->interleave_count >=
            read_length(mc_model_checker->process(),
              remote(simcall_comm_testany__get__comms(&process->simcall))))
          procstate->state = MC_DONE;

        if (*value != -1 || start_count == 0)
          return &process->simcall;

        break;
      }

      case SIMCALL_COMM_WAIT: {
        smx_synchro_t remote_act = simcall_comm_wait__get__comm(&process->simcall);
        s_smx_synchro_t act;
        mc_model_checker->process().read_bytes(
          &act, sizeof(act), remote(remote_act));
        if (act.comm.src_proc && act.comm.dst_proc)
          *value = 0;
        else if (act.comm.src_proc == nullptr && act.comm.type == SIMIX_COMM_READY
              && act.comm.detached == 1)
          *value = 0;
        else
          *value = -1;
        procstate->state = MC_DONE;
        return &process->simcall;
      }

      case SIMCALL_MC_RANDOM:
        if (procstate->state == MC_INTERLEAVE)
          *value = simcall_mc_random__get__min(&process->simcall);
        else if (state->req_num < simcall_mc_random__get__max(&process->simcall))
          *value = state->req_num + 1;
        procstate->state = MC_DONE;
        return &process->simcall;

      default:
        procstate->state = MC_DONE;
        *value = 0;
        return &process->simcall;
  }
  return nullptr;
}

smx_simcall_t MC_state_get_request(mc_state_t state, int *value)
{
  for (auto& p : mc_model_checker->process().simix_processes()) {
    smx_simcall_t res = MC_state_get_request_for_process(state, value, &p.copy);
    if (res)
      return res;
  }
  return nullptr;
}

}
