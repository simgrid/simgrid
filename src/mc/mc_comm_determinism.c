/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_state.h"
#include "mc_comm_pattern.h"
#include "mc_request.h"
#include "mc_safety.h"
#include "mc_private.h"
#include "mc_record.h"
#include "mc_smx.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc,
                                "Logging specific to MC communication determinism detection");

/********** Global variables **********/

xbt_dynar_t initial_communications_pattern;
xbt_dynar_t incomplete_communications_pattern;

/********** Static functions ***********/

static e_mc_comm_pattern_difference_t compare_comm_pattern(mc_comm_pattern_t comm1, mc_comm_pattern_t comm2) {
  if(comm1->type != comm2->type)
    return TYPE_DIFF;
  if (strcmp(comm1->rdv, comm2->rdv) != 0)
    return RDV_DIFF;
  if (comm1->src_proc != comm2->src_proc)
    return SRC_PROC_DIFF;
  if (comm1->dst_proc != comm2->dst_proc)
    return DST_PROC_DIFF;
  if (comm1->tag != comm2->tag)
    return TAG_DIFF;
  if (comm1->data_size != comm2->data_size)
    return DATA_SIZE_DIFF;
  if(comm1->data == NULL && comm2->data == NULL)
    return 0;
  if(comm1->data != NULL && comm2->data !=NULL) {
    if (!memcmp(comm1->data, comm2->data, comm1->data_size))
      return 0;
    return DATA_DIFF;
  }else{
    return DATA_DIFF;
  }
  return 0;
}

static char* print_determinism_result(e_mc_comm_pattern_difference_t diff, int process, mc_comm_pattern_t comm, unsigned int cursor) {
  char *type, *res;

  if(comm->type == SIMIX_COMM_SEND)
    type = bprintf("The send communications pattern of the process %d is different!", process - 1);
  else
    type = bprintf("The recv communications pattern of the process %d is different!", process - 1);

  switch(diff) {
  case TYPE_DIFF:
    res = bprintf("%s Different type for communication #%d", type, cursor);
    break;
  case RDV_DIFF:
    res = bprintf("%s Different rdv for communication #%d", type, cursor);
    break;
  case TAG_DIFF:
    res = bprintf("%s Different tag for communication #%d", type, cursor);
    break;
  case SRC_PROC_DIFF:
      res = bprintf("%s Different source for communication #%d", type, cursor);
    break;
  case DST_PROC_DIFF:
      res = bprintf("%s Different destination for communication #%d", type, cursor);
    break;
  case DATA_SIZE_DIFF:
    res = bprintf("%s\n Different data size for communication #%d", type, cursor);
    break;
  case DATA_DIFF:
    res = bprintf("%s\n Different data for communication #%d", type, cursor);
    break;
  default:
    res = NULL;
    break;
  }

  return res;
}

static void update_comm_pattern(mc_comm_pattern_t comm_pattern, smx_synchro_t comm)
{
  smx_process_t src_proc = MC_smx_resolve_process(comm->comm.src_proc);
  smx_process_t dst_proc = MC_smx_resolve_process(comm->comm.dst_proc);
  comm_pattern->src_proc = src_proc->pid;
  comm_pattern->dst_proc = dst_proc->pid;
  comm_pattern->src_host = MC_smx_process_get_host_name(src_proc);
  comm_pattern->dst_host = MC_smx_process_get_host_name(dst_proc);
  if (comm_pattern->data_size == -1 && comm->comm.src_buff != NULL) {
    comm_pattern->data_size = *(comm->comm.dst_buff_size);
    comm_pattern->data = xbt_malloc0(comm_pattern->data_size);
    MC_process_read_simple(&mc_model_checker->process,
      comm_pattern->data, comm->comm.src_buff, comm_pattern->data_size);
  }
}

static void deterministic_comm_pattern(int process, mc_comm_pattern_t comm, int backtracking) {

  mc_list_comm_pattern_t list_comm_pattern = (mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, process, mc_list_comm_pattern_t);

  if(!backtracking){
    mc_comm_pattern_t initial_comm = xbt_dynar_get_as(list_comm_pattern->list, list_comm_pattern->index_comm, mc_comm_pattern_t);
    e_mc_comm_pattern_difference_t diff;
    
    if((diff = compare_comm_pattern(initial_comm, comm)) != NONE_DIFF){
      if (comm->type == SIMIX_COMM_SEND){
        initial_global_state->send_deterministic = 0;
        if(initial_global_state->send_diff != NULL)
          xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = print_determinism_result(diff, process, comm, list_comm_pattern->index_comm + 1);
      }else{
        initial_global_state->recv_deterministic = 0;
        if(initial_global_state->recv_diff != NULL)
          xbt_free(initial_global_state->recv_diff);
        initial_global_state->recv_diff = print_determinism_result(diff, process, comm, list_comm_pattern->index_comm + 1);
      }
      if(_sg_mc_send_determinism && !initial_global_state->send_deterministic){
        XBT_INFO("*********************************************************");
        XBT_INFO("***** Non-send-deterministic communications pattern *****");
        XBT_INFO("*********************************************************");
        XBT_INFO("%s", initial_global_state->send_diff);
        xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = NULL;
        MC_print_statistics(mc_stats);
        xbt_abort(); 
      }else if(_sg_mc_comms_determinism && (!initial_global_state->send_deterministic && !initial_global_state->recv_deterministic)) {
        XBT_INFO("****************************************************");
        XBT_INFO("***** Non-deterministic communications pattern *****");
        XBT_INFO("****************************************************");
        XBT_INFO("%s", initial_global_state->send_diff);
        XBT_INFO("%s", initial_global_state->recv_diff);
        xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = NULL;
        xbt_free(initial_global_state->recv_diff);
        initial_global_state->recv_diff = NULL;
        MC_print_statistics(mc_stats);
        xbt_abort();
      } 
    }
  }
    
  MC_comm_pattern_free(comm);

}

/********** Non Static functions ***********/

void MC_get_comm_pattern(xbt_dynar_t list, smx_simcall_t request, e_mc_call_type_t call_type, int backtracking)
{
  mc_comm_pattern_t pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->data_size = -1;
  pattern->data = NULL;

  // Fill initial_pattern->index_comm:
  const smx_process_t issuer = MC_smx_simcall_get_issuer(request);
  mc_list_comm_pattern_t initial_pattern =
    (mc_list_comm_pattern_t) xbt_dynar_get_as(initial_communications_pattern, issuer->pid, mc_list_comm_pattern_t);
  xbt_dynar_t incomplete_pattern =
    (xbt_dynar_t) xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t);
  pattern->index =
    initial_pattern->index_comm + xbt_dynar_length(incomplete_pattern);

  
  if (call_type == MC_CALL_TYPE_SEND) {
    /* Create comm pattern */
    pattern->type = SIMIX_COMM_SEND;
    pattern->comm = simcall_comm_isend__get__result(request);
    // TODO, resolve comm
    // FIXME, remote access to rdv->name
    pattern->rdv = (pattern->comm->comm.rdv != NULL) ? strdup(pattern->comm->comm.rdv->name) : strdup(pattern->comm->comm.rdv_cpy->name);
    pattern->src_proc = MC_smx_resolve_process(pattern->comm->comm.src_proc)->pid;
    pattern->src_host = MC_smx_process_get_host_name(issuer);
    pattern->tag = ((MPI_Request)simcall_comm_isend__get__data(request))->tag;
    if(pattern->comm->comm.src_buff != NULL){
      pattern->data_size = pattern->comm->comm.src_buff_size;
      pattern->data = xbt_malloc0(pattern->data_size);
      MC_process_read_simple(&mc_model_checker->process,
        pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
    }
    if(((MPI_Request)simcall_comm_isend__get__data(request))->detached){
      if (!initial_global_state->initial_communications_pattern_done) {
        /* Store comm pattern */
        xbt_dynar_push(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, pattern->src_proc, mc_list_comm_pattern_t))->list, &pattern);
      } else {
        /* Evaluate comm determinism */
        deterministic_comm_pattern(pattern->src_proc, pattern, backtracking);
        ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, pattern->src_proc, mc_list_comm_pattern_t))->index_comm++;
      }
      return;
    }
  } else if (call_type == MC_CALL_TYPE_RECV) {                      
    pattern->type = SIMIX_COMM_RECEIVE;
    pattern->comm = simcall_comm_irecv__get__result(request);
    // TODO, remote access
    pattern->tag = ((MPI_Request)simcall_comm_irecv__get__data(request))->tag;
    pattern->rdv = (pattern->comm->comm.rdv != NULL) ? strdup(pattern->comm->comm.rdv->name) : strdup(pattern->comm->comm.rdv_cpy->name);
    pattern->dst_proc = MC_smx_resolve_process(pattern->comm->comm.dst_proc)->pid;
    // FIXME, remote process access
    pattern->dst_host = MC_smx_process_get_host_name(issuer);
  } else {
    xbt_die("Unexpected call_type %i", (int) call_type);
  }

  xbt_dynar_push((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t), &pattern);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %lu", pattern, issuer->pid);
}

void MC_complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm, unsigned int issuer, int backtracking) {
  mc_comm_pattern_t current_comm_pattern;
  unsigned int cursor = 0;
  mc_comm_pattern_t comm_pattern;
  int completed = 0;

  /* Complete comm pattern */
  xbt_dynar_foreach((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t), cursor, current_comm_pattern) {
    if (current_comm_pattern-> comm == comm) {
      update_comm_pattern(current_comm_pattern, comm);
      completed = 1;
      xbt_dynar_remove_at((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t), cursor, &comm_pattern);
      XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %u", issuer, cursor);
      break;
    }
  }
  if(!completed)
    xbt_die("Corresponding communication not found!");

  if (!initial_global_state->initial_communications_pattern_done) {
    /* Store comm pattern */
    xbt_dynar_push(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, issuer, mc_list_comm_pattern_t))->list, &comm_pattern);
  } else {
    /* Evaluate comm determinism */
    deterministic_comm_pattern(issuer, comm_pattern, backtracking);
    ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, issuer, mc_list_comm_pattern_t))->index_comm++;
  }
}


/************************ Main algorithm ************************/

void MC_pre_modelcheck_comm_determinism(void)
{
  MC_SET_MC_HEAP;

  mc_state_t initial_state = NULL;
  smx_process_t process;
  int i;

  if (_sg_mc_visited > 0)
    visited_states = xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);
 
  initial_communications_pattern = xbt_dynar_new(sizeof(mc_list_comm_pattern_t), MC_list_comm_pattern_free_voidp);
  for (i=0; i < MC_smx_get_maxpid(); i++){
    mc_list_comm_pattern_t process_list_pattern = xbt_new0(s_mc_list_comm_pattern_t, 1);
    process_list_pattern->list = xbt_dynar_new(sizeof(mc_comm_pattern_t), MC_comm_pattern_free_voidp);
    process_list_pattern->index_comm = 0;
    xbt_dynar_insert_at(initial_communications_pattern, i, &process_list_pattern);
  }
  incomplete_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i < MC_smx_get_maxpid(); i++){
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), NULL);
    xbt_dynar_insert_at(incomplete_communications_pattern, i, &process_pattern);
  }

  initial_state = MC_state_new();
  MC_SET_STD_HEAP;
  
  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_SET_MC_HEAP;

  /* Get an enabled process and insert it in the interleave set of the initial state */
  MC_EACH_SIMIX_PROCESS(process,
    if (MC_process_is_enabled(process)) {
      MC_state_interleave_process(initial_state, process);
    }
  );

  xbt_fifo_unshift(mc_stack, initial_state);

  MC_SET_STD_HEAP;

}

void MC_modelcheck_comm_determinism(void)
{

  char *req_str = NULL;
  int value;
  mc_visited_state_t visited_state = NULL;
  smx_simcall_t req = NULL;
  smx_process_t process = NULL;
  mc_state_t state = NULL, next_state = NULL;

  while (xbt_fifo_size(mc_stack) > 0) {

    /* Get current state */
    state = (mc_state_t) xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth = %d (state = %d, interleaved processes = %d)",
              xbt_fifo_size(mc_stack), state->num,
              MC_state_interleave_size(state));

    /* Update statistics */
    mc_stats->visited_states++;

    if ((xbt_fifo_size(mc_stack) <= _sg_mc_max_depth)
        && (req = MC_state_get_request(state, &value))
        && (visited_state == NULL)) {

      req_str = MC_request_to_string(req, value);  
      XBT_DEBUG("Execute: %s", req_str);                 
      xbt_free(req_str);
      
      if (dot_output != NULL) {
        MC_SET_MC_HEAP;
        req_str = MC_request_get_dot_output(req, value);
        MC_SET_STD_HEAP;
      }

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
        call = MC_get_call_type(req);
      }

      /* Answer the request */
      MC_simcall_handle(req, value);    /* After this call req is no longer useful */

      MC_SET_MC_HEAP;
      if(!initial_global_state->initial_communications_pattern_done)
        MC_handle_comm_pattern(call, req, value, initial_communications_pattern, 0);
      else
        MC_handle_comm_pattern(call, req, value, NULL, 0);
      MC_SET_STD_HEAP;

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_MC_HEAP;

      next_state = MC_state_new();

      if ((visited_state = is_visited_state(next_state)) == NULL) {

        /* Get enabled processes and insert them in the interleave set of the next state */
        MC_EACH_SIMIX_PROCESS(process,
          if (MC_process_is_enabled(process)) {
            MC_state_interleave_process(next_state, process);
          }
        );

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,  next_state->num, req_str);

      } else {

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, visited_state->other_num == -1 ? visited_state->num : visited_state->other_num, req_str);

      }

      xbt_fifo_unshift(mc_stack, next_state);

      if (dot_output != NULL)
        xbt_free(req_str);

      MC_SET_STD_HEAP;

    } else {

      if (xbt_fifo_size(mc_stack) > _sg_mc_max_depth) {
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      } else if (visited_state != NULL) {
        XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.", visited_state->other_num == -1 ? visited_state->num : visited_state->other_num);
      } else {
        XBT_DEBUG("There are no more processes to interleave. (depth %d)", xbt_fifo_size(mc_stack));
      }

      MC_SET_MC_HEAP;

      if (!initial_global_state->initial_communications_pattern_done) 
        initial_global_state->initial_communications_pattern_done = 1;

      /* Trash the current state, no longer needed */
      xbt_fifo_shift(mc_stack);
      MC_state_delete(state, !state->in_visited_states ? 1 : 0);
      XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);

      MC_SET_STD_HEAP;

      visited_state = NULL;

      /* Check for deadlocks */
      if (MC_deadlock_check()) {
        MC_show_deadlock(NULL);
        return;
      }

      MC_SET_MC_HEAP;

      while ((state = xbt_fifo_shift(mc_stack)) != NULL) {
        if (MC_state_interleave_size(state) && xbt_fifo_size(mc_stack) < _sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);
          xbt_fifo_unshift(mc_stack, state);
          MC_SET_STD_HEAP;

          MC_replay(mc_stack);

          XBT_DEBUG("Back-tracking to state %d at depth %d done", state->num, xbt_fifo_size(mc_stack));

          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);
          MC_state_delete(state, !state->in_visited_states ? 1 : 0);
        }
      }

      MC_SET_STD_HEAP;
    }
  }

  MC_print_statistics(mc_stats);
  MC_SET_STD_HEAP;

  return;
}
