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

static void comm_pattern_free(mc_comm_pattern_t p)
{
  xbt_free(p->rdv);
  xbt_free(p->data);
  xbt_free(p);
  p = NULL;
}

static void list_comm_pattern_free(mc_list_comm_pattern_t l)
{
  xbt_dynar_free(&(l->list));
  xbt_free(l);
  l = NULL;
}

static e_mc_comm_pattern_difference_t compare_comm_pattern(mc_comm_pattern_t comm1, mc_comm_pattern_t comm2) {
  if(comm1->type != comm2->type)
    return TYPE_DIFF;
  if (strcmp(comm1->rdv, comm2->rdv) != 0)
    return RDV_DIFF;
  if (comm1->src_proc != comm2->src_proc)
    return SRC_PROC_DIFF;
  if (comm1->dst_proc != comm2->dst_proc)
    return DST_PROC_DIFF;
  if (comm1->data_size != comm2->data_size)
    return DATA_SIZE_DIFF;
  if(comm1->data == NULL && comm2->data == NULL)
    return 0;
  /*if(comm1->data != NULL && comm2->data !=NULL) {
    if (!memcmp(comm1->data, comm2->data, comm1->data_size))
      return 0;
    return DATA_DIFF;
  }else{
    return DATA_DIFF;
  }*/
  return 0;
}

static void print_determinism_result(e_mc_comm_pattern_difference_t diff, int process, mc_comm_pattern_t comm, unsigned int cursor) {
  if (_sg_mc_comms_determinism && !initial_global_state->comm_deterministic) {
    XBT_INFO("****************************************************");
    XBT_INFO("***** Non-deterministic communications pattern *****");
    XBT_INFO("****************************************************");
    XBT_INFO("The communications pattern of the process %d is different!",  process);
    switch(diff) {
    case TYPE_DIFF:
      XBT_INFO("Different communication type for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case RDV_DIFF:
      XBT_INFO("Different communication rdv for communication %s at index %d",  comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case SRC_PROC_DIFF:
        XBT_INFO("Different communication source process for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DST_PROC_DIFF:
        XBT_INFO("Different communication destination process for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DATA_SIZE_DIFF:
      XBT_INFO("Different communication data size for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DATA_DIFF:
      XBT_INFO("Different communication data for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    default:
      break;
    }
    MC_print_statistics(mc_stats);
    xbt_abort();
  } else if (_sg_mc_send_determinism && !initial_global_state->send_deterministic) {
    XBT_INFO("*********************************************************");
    XBT_INFO("***** Non-send-deterministic communications pattern *****");
    XBT_INFO("*********************************************************");
    XBT_INFO("The communications pattern of the process %d is different!",  process);
    switch(diff) {
    case TYPE_DIFF:
      XBT_INFO("Different communication type for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case RDV_DIFF:
      XBT_INFO("Different communication rdv for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case SRC_PROC_DIFF:
        XBT_INFO("Different communication source process for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DST_PROC_DIFF:
        XBT_INFO("Different communication destination process for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DATA_SIZE_DIFF:
      XBT_INFO("Different communication data size for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    case DATA_DIFF:
      XBT_INFO("Different communication data for communication %s at index %d", comm->type == SIMIX_COMM_SEND ? "Send" : "Recv", cursor);
      break;
    default:
      break;
    }
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

static void print_communications_pattern()
{
  unsigned int cursor = 0, cursor2 = 0;
  mc_comm_pattern_t current_comm;
  mc_list_comm_pattern_t current_list;
  unsigned int current_process = 1;
  while (current_process < simix_process_maxpid) {
    current_list = (mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, current_process, mc_list_comm_pattern_t);
    XBT_INFO("Communications from the process %u:", current_process);
    while(cursor2 < current_list->index_comm){
      current_comm = (mc_comm_pattern_t)xbt_dynar_get_as(current_list->list, cursor2, mc_comm_pattern_t);
      if (current_comm->type == SIMIX_COMM_SEND) {
        XBT_INFO("(%u) [(%lu) %s -> (%lu) %s] %s ", cursor,current_comm->src_proc,
                 current_comm->src_host, current_comm->dst_proc,
                 current_comm->dst_host, "iSend");
      } else {
        XBT_INFO("(%u) [(%lu) %s <- (%lu) %s] %s ", cursor, current_comm->dst_proc,
                 current_comm->dst_host, current_comm->src_proc,
                 current_comm->src_host, "iRecv");
      }
      cursor2++;
    }
    current_process++;
    cursor = 0;
    cursor2 = 0;
  }
}

static void print_incomplete_communications_pattern(){
  unsigned int cursor = 0;
  unsigned int current_process = 1;
  xbt_dynar_t current_pattern;
  mc_comm_pattern_t comm;
  while (current_process < simix_process_maxpid) {
    current_pattern = (xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, current_process, xbt_dynar_t);
    XBT_INFO("Incomplete communications from the process %u:", current_process);
    xbt_dynar_foreach(current_pattern, cursor, comm) {
      XBT_DEBUG("(%u) Communication %p", cursor, comm);
    }
    current_process++;
    cursor = 0;
  }
}

// FIXME, remote comm
static void update_comm_pattern(mc_comm_pattern_t comm_pattern, smx_synchro_t comm)
{
  mc_process_t process = &mc_model_checker->process;
  void *addr_pointed;
  comm_pattern->src_proc = comm->comm.src_proc->pid;
  comm_pattern->dst_proc = comm->comm.dst_proc->pid;
  // TODO, resolve host name
  comm_pattern->src_host = MC_smx_process_get_host_name(MC_smx_resolve_process(comm->comm.src_proc));
  comm_pattern->dst_host = MC_smx_process_get_host_name(MC_smx_resolve_process(comm->comm.dst_proc));
  if (comm_pattern->data_size == -1 && comm->comm.src_buff != NULL) {
    comm_pattern->data_size = *(comm->comm.dst_buff_size);
    comm_pattern->data = xbt_malloc0(comm_pattern->data_size);
    addr_pointed = *(void **) comm->comm.src_buff;
    if (addr_pointed > (void*) process->heap_address
        && addr_pointed < MC_process_get_heap(process)->breakval)
      memcpy(comm_pattern->data, addr_pointed, comm_pattern->data_size);
    else
      memcpy(comm_pattern->data, comm->comm.src_buff, comm_pattern->data_size);
  }
}

static void deterministic_comm_pattern(int process, mc_comm_pattern_t comm, int backtracking) {

  mc_list_comm_pattern_t list_comm_pattern = (mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, process, mc_list_comm_pattern_t);

  if(!backtracking){
    mc_comm_pattern_t initial_comm = xbt_dynar_get_as(list_comm_pattern->list, comm->index, mc_comm_pattern_t);
    e_mc_comm_pattern_difference_t diff;
    
    if((diff = compare_comm_pattern(initial_comm, comm)) != NONE_DIFF){
      if (comm->type == SIMIX_COMM_SEND)
        initial_global_state->send_deterministic = 0;
      initial_global_state->comm_deterministic = 0;
      print_determinism_result(diff, process, comm, list_comm_pattern->index_comm + 1);
    }
  }
    
  list_comm_pattern->index_comm++;
  comm_pattern_free(comm);

}

/********** Non Static functions ***********/

void comm_pattern_free_voidp(void *p) {
  comm_pattern_free((mc_comm_pattern_t) * (void **) p);
}

void list_comm_pattern_free_voidp(void *p) {
  list_comm_pattern_free((mc_list_comm_pattern_t) * (void **) p);
}

void get_comm_pattern(const xbt_dynar_t list, const smx_simcall_t request, const e_mc_call_type_t call_type)
{
  mc_process_t process = &mc_model_checker->process;
  mc_comm_pattern_t pattern = NULL;
  pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->data_size = -1;
  pattern->data = NULL;

  const smx_process_t issuer = MC_smx_simcall_get_issuer(request);
  mc_list_comm_pattern_t initial_pattern =
    (mc_list_comm_pattern_t) xbt_dynar_get_as(initial_communications_pattern, issuer->pid, mc_list_comm_pattern_t);
  xbt_dynar_t incomplete_pattern =
    (xbt_dynar_t) xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t);
  pattern->index =
    initial_pattern->index_comm + xbt_dynar_length(incomplete_pattern);
  
  void *addr_pointed;
  
  if (call_type == MC_CALL_TYPE_SEND) {
    /* Create comm pattern */
    pattern->type = SIMIX_COMM_SEND;
    pattern->comm = simcall_comm_isend__get__result(request);
    pattern->rdv = (pattern->comm->comm.rdv != NULL) ? strdup(pattern->comm->comm.rdv->name) : strdup(pattern->comm->comm.rdv_cpy->name);
    pattern->src_proc = pattern->comm->comm.src_proc->pid;
    // FIXME, get remote host name
    pattern->src_host = MC_smx_process_get_host_name(issuer);
    if(pattern->comm->comm.src_buff != NULL){
      pattern->data_size = pattern->comm->comm.src_buff_size;
      pattern->data = xbt_malloc0(pattern->data_size);
      addr_pointed = *(void **) pattern->comm->comm.src_buff;
      if (addr_pointed > (void*) process->heap_address
          && addr_pointed < MC_process_get_heap(process)->breakval)
        memcpy(pattern->data, addr_pointed, pattern->data_size);
      else
        memcpy(pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
    }
  } else if (call_type == MC_CALL_TYPE_RECV) {                      
    pattern->type = SIMIX_COMM_RECEIVE;
    pattern->comm = simcall_comm_irecv__get__result(request);
    pattern->rdv = (pattern->comm->comm.rdv != NULL) ? strdup(pattern->comm->comm.rdv->name) : strdup(pattern->comm->comm.rdv_cpy->name);
    pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
    // FIXME, remote process access
    pattern->dst_host = MC_smx_process_get_host_name(issuer);
  } else {
    xbt_die("Unexpected call_type %i", (int) call_type);
  }

  xbt_dynar_push((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t), &pattern);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %lu", pattern, issuer->pid);
}

void complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm, int backtracking) {

  mc_comm_pattern_t current_comm_pattern;
  unsigned int cursor = 0;
  unsigned int src = comm->comm.src_proc->pid;
  unsigned int dst = comm->comm.dst_proc->pid;
  mc_comm_pattern_t src_comm_pattern;
  mc_comm_pattern_t dst_comm_pattern;
  int src_completed = 0, dst_completed = 0;

  /* Complete comm pattern */
  xbt_dynar_foreach((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, src, xbt_dynar_t), cursor, current_comm_pattern) {
    if (current_comm_pattern-> comm == comm) {
      update_comm_pattern(current_comm_pattern, comm);
      src_completed = 1;
      xbt_dynar_remove_at((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, src, xbt_dynar_t), cursor, &src_comm_pattern);
      XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %u", src, cursor);
      break;
    }
  }
  if(!src_completed)
    xbt_die("Corresponding communication for the source process not found!");

  cursor = 0;

  xbt_dynar_foreach((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, dst, xbt_dynar_t), cursor, current_comm_pattern) {
    if (current_comm_pattern-> comm == comm) {
      update_comm_pattern(current_comm_pattern, comm);
      dst_completed = 1;
      xbt_dynar_remove_at((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, dst, xbt_dynar_t), cursor, &dst_comm_pattern);
      XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %u", dst, cursor);
      break;
    }
  }
  if(!dst_completed)
    xbt_die("Corresponding communication for the destination process not found!");
  
  if (!initial_global_state->initial_communications_pattern_done) {
    /* Store comm pattern */
    if(src_comm_pattern->index < xbt_dynar_length(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, src, mc_list_comm_pattern_t))->list)){
      xbt_dynar_set(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, src, mc_list_comm_pattern_t))->list, src_comm_pattern->index, &src_comm_pattern);
      ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, src, mc_list_comm_pattern_t))->list->used++;
    } else {
      xbt_dynar_insert_at(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, src, mc_list_comm_pattern_t))->list, src_comm_pattern->index, &src_comm_pattern);
    }
    
    if(dst_comm_pattern->index < xbt_dynar_length(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, dst, mc_list_comm_pattern_t))->list)) {
      xbt_dynar_set(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, dst, mc_list_comm_pattern_t))->list, dst_comm_pattern->index, &dst_comm_pattern);
      ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, dst, mc_list_comm_pattern_t))->list->used++;
    } else {
      xbt_dynar_insert_at(((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, dst, mc_list_comm_pattern_t))->list, dst_comm_pattern->index, &dst_comm_pattern);
    }
    ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, src, mc_list_comm_pattern_t))->index_comm++;
    ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, dst, mc_list_comm_pattern_t))->index_comm++;
  } else {
    /* Evaluate comm determinism */
    deterministic_comm_pattern(src, src_comm_pattern, backtracking);
    deterministic_comm_pattern(dst, dst_comm_pattern, backtracking);
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
 
  initial_communications_pattern = xbt_dynar_new(sizeof(mc_list_comm_pattern_t), list_comm_pattern_free_voidp);
  for (i=0; i<simix_process_maxpid; i++){
    mc_list_comm_pattern_t process_list_pattern = xbt_new0(s_mc_list_comm_pattern_t, 1);
    process_list_pattern->list = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
    process_list_pattern->index_comm = 0;
    xbt_dynar_insert_at(initial_communications_pattern, i, &process_list_pattern);
  }
  incomplete_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i<simix_process_maxpid; i++){
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
        call = mc_get_call_type(req);
      }

      /* Answer the request */
      MC_simcall_handle(req, value);    /* After this call req is no longer useful */

      MC_SET_MC_HEAP;
      if(!initial_global_state->initial_communications_pattern_done)
        handle_comm_pattern(call, req, value, initial_communications_pattern, 0);
      else
        handle_comm_pattern(call, req, value, NULL, 0);
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
