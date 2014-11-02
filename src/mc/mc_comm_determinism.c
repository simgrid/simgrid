/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc,
                                "Logging specific to MC communication determinism detection");

/********** Global variables **********/

xbt_dynar_t initial_communications_pattern;
xbt_dynar_t incomplete_communications_pattern;
xbt_dynar_t communications_pattern;
int nb_comm_pattern;

/********** Static functions ***********/

static void comm_pattern_free(mc_comm_pattern_t p)
{
  xbt_free(p->rdv);
  xbt_free(p->data);
  xbt_free(p);
  p = NULL;
}

static void comm_pattern_free_voidp(void *p)
{
  comm_pattern_free((mc_comm_pattern_t) * (void **) p);
}

static mc_comm_pattern_t get_comm_pattern_from_idx(xbt_dynar_t pattern,
                                                   unsigned int *idx,
                                                   e_smx_comm_type_t type,
                                                   unsigned long proc)
{
  mc_comm_pattern_t current_comm;
  while (*idx < xbt_dynar_length(pattern)) {
    current_comm =
        (mc_comm_pattern_t) xbt_dynar_get_as(pattern, *idx, mc_comm_pattern_t);
    if (current_comm->type == type && type == SIMIX_COMM_SEND) {
      if (current_comm->src_proc == proc)
        return current_comm;
    } else if (current_comm->type == type && type == SIMIX_COMM_RECEIVE) {
      if (current_comm->dst_proc == proc)
        return current_comm;
    }
    (*idx)++;
  }
  return NULL;
}

static int compare_comm_pattern(mc_comm_pattern_t comm1,
                                mc_comm_pattern_t comm2)
{
  if (strcmp(comm1->rdv, comm2->rdv) != 0)
    return 1;
  if (comm1->src_proc != comm2->src_proc)
    return 1;
  if (comm1->dst_proc != comm2->dst_proc)
    return 1;
  if (comm1->data_size != comm2->data_size)
    return 1;
  if (memcmp(comm1->data, comm2->data, comm1->data_size) != 0)
    return 1;
  return 0;
}

static void deterministic_pattern(xbt_dynar_t pattern, int partial)
{

  unsigned int cursor = 0, send_index = 0, recv_index = 0;
  mc_comm_pattern_t comm1, comm2;
  unsigned int current_process = 1; /* Process 0 corresponds to maestro */
  unsigned int nb_comms1, nb_comms2;
  xbt_dynar_t process_comms_pattern1, process_comms_pattern2; 
  
  while (current_process < simix_process_maxpid) {
    process_comms_pattern1 = (xbt_dynar_t)xbt_dynar_get_as(initial_communications_pattern, current_process, xbt_dynar_t);
    process_comms_pattern2 = (xbt_dynar_t)xbt_dynar_get_as(pattern, current_process, xbt_dynar_t);
    nb_comms1 = xbt_dynar_length(process_comms_pattern1);
    nb_comms2 = xbt_dynar_length(process_comms_pattern2);
    if(!xbt_dynar_is_empty((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, current_process, xbt_dynar_t)))
      xbt_die("Damn ! Some communications from the process %u are incomplete (%lu)! That means one or several simcalls are not handle.", current_process, xbt_dynar_length((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, current_process, xbt_dynar_t)));
    if (!partial && (nb_comms1 != nb_comms2)) {
      XBT_INFO("The total number of communications is different between the compared patterns for the process %u.\n Communication determinism verification for this process cannot be performed.", current_process);
      initial_global_state->send_deterministic = -1;
      initial_global_state->comm_deterministic = -1;
    } else {
      while (cursor < nb_comms2) {
        comm1 = (mc_comm_pattern_t)xbt_dynar_get_as(process_comms_pattern1, cursor, mc_comm_pattern_t);
        if (comm1->type == SIMIX_COMM_SEND) {
          comm2 = get_comm_pattern_from_idx(process_comms_pattern2, &send_index, comm1->type, current_process);
          if (compare_comm_pattern(comm1, comm2)) {
            XBT_INFO("The communications pattern of the process %u is different! (Different communication : %u)", current_process, cursor+1);
            initial_global_state->send_deterministic = 0;
            initial_global_state->comm_deterministic = 0;
            return;
          }
          send_index++;
        } else if (comm1->type == SIMIX_COMM_RECEIVE) {
          comm2 = get_comm_pattern_from_idx(process_comms_pattern2, &recv_index, comm1->type, current_process);
          if (compare_comm_pattern(comm1, comm2)) {
            initial_global_state->comm_deterministic = 0;
            if (!_sg_mc_send_determinism){
              XBT_INFO("The communications pattern of the process %u is different! (Different communication : %u)", current_process, cursor+1);
              return;
            }
          }
          recv_index++;
        }
        cursor++;
      }
    }
    current_process++;
    cursor = 0;
    send_index = 0;
    recv_index = 0;
  }
}

static void print_communications_pattern(xbt_dynar_t comms_pattern)
{
  unsigned int cursor = 0;
  mc_comm_pattern_t current_comm;
  unsigned int current_process = 1;
  xbt_dynar_t current_pattern;
  while (current_process < simix_process_maxpid) {
    current_pattern = (xbt_dynar_t)xbt_dynar_get_as(comms_pattern, current_process, xbt_dynar_t);
    XBT_INFO("Communications from the process %u:", current_process);
    xbt_dynar_foreach(current_pattern, cursor, current_comm) {
      if (current_comm->type == SIMIX_COMM_SEND) {
        XBT_INFO("[(%lu) %s -> (%lu) %s] %s ", current_comm->src_proc,
                 current_comm->src_host, current_comm->dst_proc,
                 current_comm->dst_host, "iSend");
      } else {
        XBT_INFO("[(%lu) %s <- (%lu) %s] %s ", current_comm->dst_proc,
                 current_comm->dst_host, current_comm->src_proc,
                 current_comm->src_host, "iRecv");
      }
    }
    current_process++;
    cursor = 0;
  }
}

static void update_comm_pattern(mc_comm_pattern_t comm_pattern, smx_synchro_t comm)
{
  void *addr_pointed;
  comm_pattern->src_proc = comm->comm.src_proc->pid;
  comm_pattern->dst_proc = comm->comm.dst_proc->pid;
  comm_pattern->src_host =
    simcall_host_get_name(comm->comm.src_proc->smx_host);
  comm_pattern->dst_host =
    simcall_host_get_name(comm->comm.dst_proc->smx_host);
  if (comm_pattern->data_size == -1) {
    comm_pattern->data_size = *(comm->comm.dst_buff_size);
    comm_pattern->data = xbt_malloc0(comm_pattern->data_size);
    addr_pointed = *(void **) comm->comm.src_buff;
    if (addr_pointed > (void*) std_heap && addr_pointed < std_heap->breakval)
      memcpy(comm_pattern->data, addr_pointed, comm_pattern->data_size);
    else
      memcpy(comm_pattern->data, comm->comm.src_buff, comm_pattern->data_size);
  }
}

/********** Non Static functions ***********/

void get_comm_pattern(xbt_dynar_t list, smx_simcall_t request, mc_call_type call_type)
{
  mc_comm_pattern_t pattern = NULL;
  pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->num = ++nb_comm_pattern;
  pattern->data_size = -1;
  void *addr_pointed;
  if (call_type == MC_CALL_TYPE_SEND) {              // ISEND
    pattern->type = SIMIX_COMM_SEND;
    pattern->comm = simcall_comm_isend__get__result(request);
    pattern->src_proc = pattern->comm->comm.src_proc->pid;
    pattern->src_host = simcall_host_get_name(request->issuer->smx_host);
    pattern->data_size = pattern->comm->comm.src_buff_size;
    pattern->data = xbt_malloc0(pattern->data_size);
    addr_pointed = *(void **) pattern->comm->comm.src_buff;
    if (addr_pointed > (void*) std_heap && addr_pointed < std_heap->breakval)
      memcpy(pattern->data, addr_pointed, pattern->data_size);
    else
      memcpy(pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
  } else if (call_type == MC_CALL_TYPE_RECV) {                      // IRECV
    pattern->type = SIMIX_COMM_RECEIVE;
    pattern->comm = simcall_comm_irecv__get__result(request);
    pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
    pattern->dst_host = simcall_host_get_name(request->issuer->smx_host);
  } else {
    xbt_die("Unexpected call_type %i", (int) call_type);
  }

  if (pattern->comm->comm.rdv != NULL)
    pattern->rdv = strdup(pattern->comm->comm.rdv->name);
  else
    pattern->rdv = strdup(pattern->comm->comm.rdv_cpy->name);

  xbt_dynar_push((xbt_dynar_t)xbt_dynar_get_as(list, request->issuer->pid, xbt_dynar_t), &pattern);

  xbt_dynar_push_as((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, request->issuer->pid, xbt_dynar_t), int, xbt_dynar_length((xbt_dynar_t)xbt_dynar_get_as(list, request->issuer->pid, xbt_dynar_t)) - 1);

}

void complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm)
{
  mc_comm_pattern_t current_comm_pattern;
  unsigned int cursor = 0;
  int index;
  unsigned int src = comm->comm.src_proc->pid;
  unsigned int dst = comm->comm.dst_proc->pid;
  int src_completed = 0, dst_completed = 0;

  /* Looking for the corresponding communication in the comm pattern list of the src process */
  xbt_dynar_foreach((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, src, xbt_dynar_t), cursor, index){
    current_comm_pattern = (mc_comm_pattern_t) xbt_dynar_get_as((xbt_dynar_t)xbt_dynar_get_as(list, src, xbt_dynar_t), index, mc_comm_pattern_t);
    if(current_comm_pattern->comm == comm){
      update_comm_pattern(current_comm_pattern, comm);
      xbt_dynar_remove_at((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, src, xbt_dynar_t), cursor, NULL);
      src_completed = 1;
      break;
    }
  }

  if(!src_completed)
    xbt_die("Corresponding communication for the source process not found!");

  cursor = 0;

  /* Looking for the corresponding communication in the comm pattern list of the dst process */
  xbt_dynar_foreach((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, dst, xbt_dynar_t), cursor, index){
    current_comm_pattern = (mc_comm_pattern_t) xbt_dynar_get_as((xbt_dynar_t)xbt_dynar_get_as(list, dst, xbt_dynar_t), index, mc_comm_pattern_t);
    if(current_comm_pattern->comm == comm){
      update_comm_pattern(current_comm_pattern, comm);
      xbt_dynar_remove_at((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, dst, xbt_dynar_t), cursor, NULL);
      dst_completed = 1;
      break;
    }
  }

  if(!dst_completed)
    xbt_die("Corresponding communication for the dest process not found!");


}

/************************ Main algorithm ************************/

void MC_pre_modelcheck_comm_determinism(void)
{

  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  mc_state_t initial_state = NULL;
  smx_process_t process;
  int i;

  if (!mc_mem_set)
    MC_SET_MC_HEAP;

  if (_sg_mc_visited > 0)
    visited_states = xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);
 
  initial_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i<simix_process_maxpid; i++){
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
    xbt_dynar_insert_at(initial_communications_pattern, i, &process_pattern);
  }
  communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i<simix_process_maxpid; i++){
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
    xbt_dynar_insert_at(communications_pattern, i, &process_pattern);
  }
  incomplete_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i<simix_process_maxpid; i++){
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(int), NULL);
    xbt_dynar_insert_at(incomplete_communications_pattern, i, &process_pattern);
  }

  nb_comm_pattern = 0;

  initial_state = MC_state_new();

  MC_SET_STD_HEAP;

  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_SET_MC_HEAP;

  /* Get an enabled process and insert it in the interleave set of the initial state */
  xbt_swag_foreach(process, simix_global->process_list) {
    if (MC_process_is_enabled(process)) {
      MC_state_interleave_process(initial_state, process);
    }
  }

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
  xbt_dynar_t current_pattern;

  while (xbt_fifo_size(mc_stack) > 0) {

    /* Get current state */
    state =
        (mc_state_t)
        xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth = %d (state = %d, interleaved processes = %d)",
              xbt_fifo_size(mc_stack), state->num,
              MC_state_interleave_size(state));

    /* Update statistics */
    mc_stats->visited_states++;

    if ((xbt_fifo_size(mc_stack) <= _sg_mc_max_depth)
        && (req = MC_state_get_request(state, &value))
        && (visited_state == NULL)) {

      MC_LOG_REQUEST(mc_comm_determinism, req, value);

      if (dot_output != NULL) {
        MC_SET_MC_HEAP;
        req_str = MC_request_get_dot_output(req, value);
        MC_SET_STD_HEAP;
      }

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      mc_call_type call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
        call = mc_get_call_type(req);
      }

      /* Answer the request */
      SIMIX_simcall_handle(req, value);    /* After this call req is no longer useful */

      MC_SET_MC_HEAP;
      current_pattern = !initial_global_state->initial_communications_pattern_done ? initial_communications_pattern : communications_pattern; 
      mc_update_comm_pattern(call, req, value, current_pattern);
      MC_SET_STD_HEAP;

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_MC_HEAP;

      next_state = MC_state_new();

      if ((visited_state = is_visited_state()) == NULL) {

        /* Get enabled processes and insert them in the interleave set of the next state */
        xbt_swag_foreach(process, simix_global->process_list) {
          if (MC_process_is_enabled(process)) {
            MC_state_interleave_process(next_state, process);
          }
        }

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,
                  next_state->num, req_str);

      } else {

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,
                  visited_state->other_num == -1 ? visited_state->num : visited_state->other_num, req_str);

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

      if (initial_global_state->initial_communications_pattern_done) {
        if (!visited_state) {
          deterministic_pattern(communications_pattern, 0);
        } else {
          deterministic_pattern(communications_pattern, 1);
        }

        if (_sg_mc_comms_determinism && !initial_global_state->comm_deterministic) {
            XBT_INFO("****************************************************");
            XBT_INFO("***** Non-deterministic communications pattern *****");
            XBT_INFO("****************************************************");
            XBT_INFO("** Initial communications pattern (per process): **");
            print_communications_pattern(initial_communications_pattern);
            XBT_INFO("** Communications pattern counter-example (per process): **");
            print_communications_pattern(communications_pattern);
            MC_print_statistics(mc_stats);
            MC_SET_STD_HEAP;
            return;
          } else if (_sg_mc_send_determinism && !initial_global_state->send_deterministic) {
            XBT_INFO
                ("*********************************************************");
            XBT_INFO
                ("***** Non-send-deterministic communications pattern *****");
            XBT_INFO
                ("*********************************************************");
            XBT_INFO("** Initial communications pattern: **");
            print_communications_pattern(initial_communications_pattern);
            XBT_INFO("** Communications pattern counter-example: **");
            print_communications_pattern(communications_pattern);
            MC_print_statistics(mc_stats);
            MC_SET_STD_HEAP;
            return;
        }

      } else {
        initial_global_state->initial_communications_pattern_done = 1;
      }

      /* Trash the current state, no longer needed */
      xbt_fifo_shift(mc_stack);
      MC_state_delete(state);
      XBT_DEBUG("Delete state %d at depth %d", state->num,
                xbt_fifo_size(mc_stack) + 1);

      MC_SET_STD_HEAP;

      visited_state = NULL;

      /* Check for deadlocks */
      if (MC_deadlock_check()) {
        MC_show_deadlock(NULL);
        return;
      }

      MC_SET_MC_HEAP;

      while ((state = xbt_fifo_shift(mc_stack)) != NULL) {
        if (MC_state_interleave_size(state)
            && xbt_fifo_size(mc_stack) < _sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num,
                    xbt_fifo_size(mc_stack) + 1);
          xbt_fifo_unshift(mc_stack, state);
          MC_SET_STD_HEAP;

          MC_replay(mc_stack, -1);

          XBT_DEBUG("Back-tracking to state %d at depth %d done", state->num,
                    xbt_fifo_size(mc_stack));
          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %d", state->num,
                    xbt_fifo_size(mc_stack) + 1);
          MC_state_delete(state);
        }
      }

      MC_SET_STD_HEAP;
    }
  }

  MC_print_statistics(mc_stats);
  MC_SET_STD_HEAP;

  return;
}
