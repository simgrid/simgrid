/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>

#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_smx.h"
#include "src/mc/mc_xbt.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_pattern, mc,
                                "Logging specific to MC communication patterns");

extern "C" {

mc_comm_pattern_t MC_comm_pattern_dup(mc_comm_pattern_t comm)
{
  mc_comm_pattern_t res = xbt_new0(s_mc_comm_pattern_t, 1);
  res->index = comm->index;
  res->type = comm->type;
  res->comm_addr = comm->comm_addr;
  res->rdv = xbt_strdup(comm->rdv);
  res->data_size = -1;
  res->data = nullptr;
  if (comm->type == SIMIX_COMM_SEND) {
    res->src_proc = comm->src_proc;
    res->src_host = comm->src_host;
    if (comm->data != nullptr) {
      res->data_size = comm->data_size;
      res->data = xbt_malloc0(comm->data_size);
      memcpy(res->data, comm->data, comm->data_size);
    }
  } else {
    res->dst_proc = comm->dst_proc;
    res->dst_host = comm->dst_host;
  }
  return res;
}

xbt_dynar_t MC_comm_patterns_dup(xbt_dynar_t patterns)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(mc_comm_pattern_t), MC_comm_pattern_free_voidp);

  mc_comm_pattern_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(patterns, cursor, comm) {
    mc_comm_pattern_t copy_comm = MC_comm_pattern_dup(comm);
    xbt_dynar_push(res, &copy_comm);
  }

  return res;
}

static void MC_patterns_copy(xbt_dynar_t dest, xbt_dynar_t source)
{
  xbt_dynar_reset(dest);

  unsigned int cursor;
  mc_comm_pattern_t comm;
  xbt_dynar_foreach(source, cursor, comm) {
    mc_comm_pattern_t copy_comm = MC_comm_pattern_dup(comm);
    xbt_dynar_push(dest, &copy_comm);
  }
}

void MC_restore_communications_pattern(mc_state_t state)
{
  mc_list_comm_pattern_t list_process_comm;
  unsigned int cursor;

  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm)
    list_process_comm->index_comm = (int)xbt_dynar_get_as(state->index_comm, cursor, int);

  for (unsigned i = 0; i < MC_smx_get_maxpid(); i++)
    MC_patterns_copy(
      xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t),
      xbt_dynar_get_as(state->incomplete_comm_pattern, i, xbt_dynar_t)
    );
}

void MC_state_copy_incomplete_communications_pattern(mc_state_t state)
{
  state->incomplete_comm_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);

  for (unsigned i=0; i < MC_smx_get_maxpid(); i++) {
    xbt_dynar_t comms = xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t);
    xbt_dynar_t copy = MC_comm_patterns_dup(comms);
    xbt_dynar_insert_at(state->incomplete_comm_pattern, i, &copy);
  }
}

void MC_state_copy_index_communications_pattern(mc_state_t state)
{
  state->index_comm = xbt_dynar_new(sizeof(unsigned int), nullptr);
  mc_list_comm_pattern_t list_process_comm;
  unsigned int cursor;
  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm)
    xbt_dynar_push_as(state->index_comm, unsigned int, list_process_comm->index_comm);
}

void MC_handle_comm_pattern(
  e_mc_call_type_t call_type, smx_simcall_t req,
  int value, xbt_dynar_t pattern, int backtracking)
{

  switch(call_type) {
  case MC_CALL_TYPE_NONE:
    break;
  case MC_CALL_TYPE_SEND:
  case MC_CALL_TYPE_RECV:
    MC_get_comm_pattern(pattern, req, call_type, backtracking);
    break;
  case MC_CALL_TYPE_WAIT:
  case MC_CALL_TYPE_WAITANY:
    {
      smx_synchro_t comm_addr = nullptr;
      if (call_type == MC_CALL_TYPE_WAIT)
        comm_addr = simcall_comm_wait__get__comm(req);
      else
        // comm_addr = REMOTE(xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value, smx_synchro_t)):
        simgrid::mc::read_element(mc_model_checker->process(), &comm_addr,
          remote(simcall_comm_waitany__get__comms(req)), value, sizeof(comm_addr));
      MC_complete_comm_pattern(pattern, comm_addr,
        MC_smx_simcall_get_issuer(req)->pid, backtracking);
    }
    break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }

}

void MC_comm_pattern_free(mc_comm_pattern_t p)
{
  xbt_free(p->rdv);
  xbt_free(p->data);
  xbt_free(p);
  p = nullptr;
}

static void MC_list_comm_pattern_free(mc_list_comm_pattern_t l)
{
  xbt_dynar_free(&(l->list));
  xbt_free(l);
  l = nullptr;
}

void MC_comm_pattern_free_voidp(void *p)
{
  MC_comm_pattern_free((mc_comm_pattern_t) * (void **) p);
}

void MC_list_comm_pattern_free_voidp(void *p)
{
  MC_list_comm_pattern_free((mc_list_comm_pattern_t) * (void **) p);
}

}
