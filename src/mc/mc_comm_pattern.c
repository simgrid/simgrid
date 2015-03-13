/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string.h>

#include <xbt/sysdep.h>
#include <xbt/dynar.h>

#include "mc_comm_pattern.h"

mc_comm_pattern_t MC_comm_pattern_dup(mc_comm_pattern_t comm)
{
  mc_comm_pattern_t res = xbt_new0(s_mc_comm_pattern_t, 1);
  res->index = comm->index;
  res->type = comm->type;
  res->comm = comm->comm;
  res->rdv = strdup(comm->rdv);
  res->data_size = -1;
  res->data = NULL;
  if (comm->type == SIMIX_COMM_SEND) {
    res->src_proc = comm->src_proc;
    res->src_host = comm->src_host;
    if (comm->data != NULL) {
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
  xbt_dynar_t res = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);

  mc_comm_pattern_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(patterns, cursor, comm) {
    mc_comm_pattern_t copy_comm = MC_comm_pattern_dup(comm);
    xbt_dynar_push(res, &copy_comm);
  }

  return res;
}

void MC_restore_communications_pattern(mc_state_t state)
{
  mc_list_comm_pattern_t list_process_comm;
  unsigned int cursor;

  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm){
    list_process_comm->index_comm = (int)xbt_dynar_get_as(state->index_comm, cursor, int);
  }

  for (int i = 0; i < MC_smx_get_maxpid(); i++) {
    xbt_dynar_t initial_incomplete_process_comms = xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t);
    xbt_dynar_reset(initial_incomplete_process_comms);
    xbt_dynar_t incomplete_process_comms = xbt_dynar_get_as(state->incomplete_comm_pattern, i, xbt_dynar_t);

    mc_comm_pattern_t comm;
    xbt_dynar_foreach(incomplete_process_comms, cursor, comm) {
      mc_comm_pattern_t copy_comm = MC_comm_pattern_dup(comm);
      xbt_dynar_push(initial_incomplete_process_comms, &copy_comm);
    }

  }
}

void MC_state_copy_incomplete_communications_pattern(mc_state_t state)
{
  state->incomplete_comm_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);

  int i;
  for (i=0; i < MC_smx_get_maxpid(); i++) {
    xbt_dynar_t comms = xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t);
    xbt_dynar_t copy = MC_comm_patterns_dup(comms);
    xbt_dynar_insert_at(state->incomplete_comm_pattern, i, &copy);
  }
}

void MC_state_copy_index_communications_pattern(mc_state_t state)
{
  state->index_comm = xbt_dynar_new(sizeof(unsigned int), NULL);
  mc_list_comm_pattern_t list_process_comm;
  unsigned int cursor;
  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm){
    xbt_dynar_push_as(state->index_comm, unsigned int, list_process_comm->index_comm);
  }
}
