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

static void deterministic_pattern(xbt_dynar_t initial_pattern,
                                  xbt_dynar_t pattern)
{

  if (!xbt_dynar_is_empty(incomplete_communications_pattern))
    xbt_die
        ("Damn ! Some communications are incomplete that means one or several simcalls are not handle ... ");

  unsigned int cursor = 0, send_index = 0, recv_index = 0;
  mc_comm_pattern_t comm1, comm2;
  int comm_comparison = 0;
  int current_process = 0;
  while (current_process < simix_process_maxpid) {
    while (cursor < xbt_dynar_length(initial_pattern)) {
      comm1 =
          (mc_comm_pattern_t) xbt_dynar_get_as(initial_pattern, cursor,
                                               mc_comm_pattern_t);
      if (comm1->type == SIMIX_COMM_SEND && comm1->src_proc == current_process) {
        comm2 =
            get_comm_pattern_from_idx(pattern, &send_index, comm1->type,
                                      current_process);
        comm_comparison = compare_comm_pattern(comm1, comm2);
        if (comm_comparison == 1) {
          initial_global_state->send_deterministic = 0;
          initial_global_state->comm_deterministic = 0;
          return;
        }
        send_index++;
      } else if (comm1->type == SIMIX_COMM_RECEIVE
                 && comm1->dst_proc == current_process) {
        comm2 =
            get_comm_pattern_from_idx(pattern, &recv_index, comm1->type,
                                      current_process);
        comm_comparison = compare_comm_pattern(comm1, comm2);
        if (comm_comparison == 1) {
          initial_global_state->comm_deterministic = 0;
          if (!_sg_mc_send_determinism)
            return;
        }
        recv_index++;
      }
      cursor++;
    }
    cursor = 0;
    send_index = 0;
    recv_index = 0;
    current_process++;
  }
}

static void print_communications_pattern(xbt_dynar_t comms_pattern)
{
  unsigned int cursor = 0;
  mc_comm_pattern_t current_comm;
  xbt_dynar_foreach(comms_pattern, cursor, current_comm) {
    if (current_comm->type == SIMIX_COMM_SEND)
      XBT_INFO("[(%lu) %s -> (%lu) %s] %s ", current_comm->src_proc,
               current_comm->src_host, current_comm->dst_proc,
               current_comm->dst_host, "iSend");
    else
      XBT_INFO("[(%lu) %s <- (%lu) %s] %s ", current_comm->dst_proc,
               current_comm->dst_host, current_comm->src_proc,
               current_comm->src_host, "iRecv");
  }
}


/********** Non Static functions ***********/

void get_comm_pattern(xbt_dynar_t list, smx_simcall_t request, int call)
{
  mc_comm_pattern_t pattern = NULL;
  pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->num = ++nb_comm_pattern;
  pattern->data_size = -1;
  void *addr_pointed;
  if (call == 1) {              // ISEND
    pattern->comm = simcall_comm_isend__get__result(request);
    pattern->type = SIMIX_COMM_SEND;
    pattern->src_proc = pattern->comm->comm.src_proc->pid;
    pattern->src_host = simcall_host_get_name(request->issuer->smx_host);
    pattern->data_size = pattern->comm->comm.src_buff_size;
    pattern->data = xbt_malloc0(pattern->data_size);
    addr_pointed = *(void **) pattern->comm->comm.src_buff;
    if (addr_pointed > std_heap
        && addr_pointed < ((xbt_mheap_t) std_heap)->breakval)
      memcpy(pattern->data, addr_pointed, pattern->data_size);
    else
      memcpy(pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
  } else {                      // IRECV
    pattern->comm = simcall_comm_irecv__get__result(request);
    pattern->type = SIMIX_COMM_RECEIVE;
    pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
    pattern->dst_host =
        simcall_host_get_name(pattern->comm->comm.dst_proc->smx_host);
  }

  if (pattern->comm->comm.rdv != NULL)
    pattern->rdv = strdup(pattern->comm->comm.rdv->name);
  else
    pattern->rdv = strdup(pattern->comm->comm.rdv_cpy->name);

  xbt_dynar_push(list, &pattern);

  xbt_dynar_push_as(incomplete_communications_pattern, int,
                    xbt_dynar_length(list) - 1);

}

void complete_comm_pattern(xbt_dynar_t list, smx_action_t comm)
{
  mc_comm_pattern_t current_pattern;
  unsigned int cursor = 0;
  int index;
  int completed = 0;
  void *addr_pointed;
  xbt_dynar_foreach(incomplete_communications_pattern, cursor, index) {
    current_pattern =
        (mc_comm_pattern_t) xbt_dynar_get_as(list, index, mc_comm_pattern_t);
    if (current_pattern->comm == comm) {
      current_pattern->src_proc = comm->comm.src_proc->pid;
      current_pattern->dst_proc = comm->comm.dst_proc->pid;
      current_pattern->src_host =
          simcall_host_get_name(comm->comm.src_proc->smx_host);
      current_pattern->dst_host =
          simcall_host_get_name(comm->comm.dst_proc->smx_host);
      if (current_pattern->data_size == -1) {
        current_pattern->data_size = *(comm->comm.dst_buff_size);
        current_pattern->data = xbt_malloc0(current_pattern->data_size);
        addr_pointed = *(void **) comm->comm.src_buff;
        if (addr_pointed > std_heap
            && addr_pointed < ((xbt_mheap_t) std_heap)->breakval)
          memcpy(current_pattern->data, addr_pointed,
                 current_pattern->data_size);
        else
          memcpy(current_pattern->data, comm->comm.src_buff,
                 current_pattern->data_size);
      }
      xbt_dynar_remove_at(incomplete_communications_pattern, cursor, NULL);
      completed++;
      if (completed == 2)
        return;
      cursor--;
    }
  }
}

/************************ Main algorithm ************************/

void MC_pre_modelcheck_comm_determinism(void)
{

  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  mc_state_t initial_state = NULL;
  smx_process_t process;

  if (!mc_mem_set)
    MC_SET_MC_HEAP;

  if (_sg_mc_visited > 0)
    visited_states =
        xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);

  initial_communications_pattern =
      xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
  communications_pattern =
      xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
  incomplete_communications_pattern = xbt_dynar_new(sizeof(int), NULL);
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
  smx_simcall_t req = NULL;
  smx_process_t process = NULL;
  mc_state_t state = NULL, next_state = NULL;
  int comm_pattern = 0, visited_state = -1;
  smx_action_t current_comm;

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
        && (visited_state == -1)) {

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_comm_determinism, xbt_log_priority_debug)) {
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
        xbt_free(req_str);
      }

      MC_SET_MC_HEAP;
      if (dot_output != NULL)
        req_str = MC_request_get_dot_output(req, value);
      MC_SET_STD_HEAP;

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
        if (req->call == SIMCALL_COMM_ISEND)
          comm_pattern = 1;
        else if (req->call == SIMCALL_COMM_IRECV)
          comm_pattern = 2;
        else if (req->call == SIMCALL_COMM_WAIT)
          comm_pattern = 3;
        else if (req->call == SIMCALL_COMM_WAITANY)
          comm_pattern = 4;
      }

      /* Answer the request */
      SIMIX_simcall_pre(req, value);    /* After this call req is no longer usefull */

      MC_SET_MC_HEAP;
      if (comm_pattern == 1 || comm_pattern == 2) {
        if (!initial_global_state->initial_communications_pattern_done)
          get_comm_pattern(initial_communications_pattern, req, comm_pattern);
        else
          get_comm_pattern(communications_pattern, req, comm_pattern);
      } else if (comm_pattern == 3) {
        current_comm = simcall_comm_wait__get__comm(req);
        if (current_comm->comm.refcount == 1) { /* First wait only must be considered */
          if (!initial_global_state->initial_communications_pattern_done)
            complete_comm_pattern(initial_communications_pattern, current_comm);
          else
            complete_comm_pattern(communications_pattern, current_comm);
        }
      } else if (comm_pattern == 4) {
        current_comm =
            xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value,
                             smx_action_t);
        if (current_comm->comm.refcount == 1) { /* First wait only must be considered */
          if (!initial_global_state->initial_communications_pattern_done)
            complete_comm_pattern(initial_communications_pattern, current_comm);
          else
            complete_comm_pattern(communications_pattern, current_comm);
        }
      }
      MC_SET_STD_HEAP;

      comm_pattern = 0;

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_MC_HEAP;

      next_state = MC_state_new();

      if ((visited_state = is_visited_state()) == -1) {

        /* Get an enabled process and insert it in the interleave set of the next state */
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
                  visited_state, req_str);

      }

      xbt_fifo_unshift(mc_stack, next_state);

      if (dot_output != NULL)
        xbt_free(req_str);

      MC_SET_STD_HEAP;

    } else {

      if (xbt_fifo_size(mc_stack) > _sg_mc_max_depth) {
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      } else if (visited_state != -1) {
        XBT_DEBUG("State already visited, exploration stopped on this path.");
      } else {
        XBT_DEBUG("There are no more processes to interleave. (depth %d)",
                  xbt_fifo_size(mc_stack) + 1);
      }

      MC_SET_MC_HEAP;

      if (initial_global_state->initial_communications_pattern_done) {
        if (visited_state == -1) {
          deterministic_pattern(initial_communications_pattern,
                                communications_pattern);
          if (initial_global_state->comm_deterministic == 0
              && _sg_mc_comms_determinism) {
            XBT_INFO("****************************************************");
            XBT_INFO("***** Non-deterministic communications pattern *****");
            XBT_INFO("****************************************************");
            XBT_INFO("Initial communications pattern:");
            print_communications_pattern(initial_communications_pattern);
            XBT_INFO("Communications pattern counter-example:");
            print_communications_pattern(communications_pattern);
            MC_print_statistics(mc_stats);
            MC_SET_STD_HEAP;
            return;
          } else if (initial_global_state->send_deterministic == 0
                     && _sg_mc_send_determinism) {
            XBT_INFO
                ("*********************************************************");
            XBT_INFO
                ("***** Non-send-deterministic communications pattern *****");
            XBT_INFO
                ("*********************************************************");
            XBT_INFO("Initial communications pattern:");
            print_communications_pattern(initial_communications_pattern);
            XBT_INFO("Communications pattern counter-example:");
            print_communications_pattern(communications_pattern);
            MC_print_statistics(mc_stats);
            MC_SET_STD_HEAP;
            return;
          }
        } else {

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

      visited_state = -1;

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
