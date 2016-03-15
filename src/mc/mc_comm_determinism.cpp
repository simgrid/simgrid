/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>

#include <xbt/dynar.h>
#include <xbt/dynar.hpp>
#include <xbt/fifo.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/mc_state.h"
#include "src/mc/mc_comm_pattern.h"
#include "src/mc/mc_request.h"
#include "src/mc/mc_safety.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_record.h"
#include "src/mc/mc_smx.h"
#include "src/mc/Client.hpp"
#include "src/mc/mc_exit.h"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_comm_determinism, mc,
                                "Logging specific to MC communication determinism detection");

extern "C" {

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
  if(comm1->data == nullptr && comm2->data == NULL)
    return NONE_DIFF;
  if(comm1->data != nullptr && comm2->data !=NULL) {
    if (!memcmp(comm1->data, comm2->data, comm1->data_size))
      return NONE_DIFF;
    return DATA_DIFF;
  } else
    return DATA_DIFF;
  return NONE_DIFF;
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
    res = nullptr;
    break;
  }

  return res;
}

static void update_comm_pattern(mc_comm_pattern_t comm_pattern, smx_synchro_t comm_addr)
{
  s_smx_synchro_t comm;
  mc_model_checker->process().read(&comm, remote(comm_addr));

  smx_process_t src_proc = MC_smx_resolve_process(comm.comm.src_proc);
  smx_process_t dst_proc = MC_smx_resolve_process(comm.comm.dst_proc);
  comm_pattern->src_proc = src_proc->pid;
  comm_pattern->dst_proc = dst_proc->pid;
  comm_pattern->src_host = MC_smx_process_get_host_name(src_proc);
  comm_pattern->dst_host = MC_smx_process_get_host_name(dst_proc);
  if (comm_pattern->data_size == -1 && comm.comm.src_buff != nullptr) {
    size_t buff_size;
    mc_model_checker->process().read(
      &buff_size, remote(comm.comm.dst_buff_size));
    comm_pattern->data_size = buff_size;
    comm_pattern->data = xbt_malloc0(comm_pattern->data_size);
    mc_model_checker->process().read_bytes(
      comm_pattern->data, comm_pattern->data_size,
      remote(comm.comm.src_buff));
  }
}

static void deterministic_comm_pattern(int process, mc_comm_pattern_t comm, int backtracking) {

  mc_list_comm_pattern_t list =
    xbt_dynar_get_as(initial_communications_pattern, process, mc_list_comm_pattern_t);

  if(!backtracking){
    mc_comm_pattern_t initial_comm =
      xbt_dynar_get_as(list->list, list->index_comm, mc_comm_pattern_t);
    e_mc_comm_pattern_difference_t diff =
      compare_comm_pattern(initial_comm, comm);

    if (diff != NONE_DIFF) {
      if (comm->type == SIMIX_COMM_SEND){
        initial_global_state->send_deterministic = 0;
        if(initial_global_state->send_diff != nullptr)
          xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = print_determinism_result(diff, process, comm, list->index_comm + 1);
      }else{
        initial_global_state->recv_deterministic = 0;
        if(initial_global_state->recv_diff != nullptr)
          xbt_free(initial_global_state->recv_diff);
        initial_global_state->recv_diff = print_determinism_result(diff, process, comm, list->index_comm + 1);
      }
      if(_sg_mc_send_determinism && !initial_global_state->send_deterministic){
        XBT_INFO("*********************************************************");
        XBT_INFO("***** Non-send-deterministic communications pattern *****");
        XBT_INFO("*********************************************************");
        XBT_INFO("%s", initial_global_state->send_diff);
        xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = nullptr;
        MC_print_statistics(mc_stats);
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      }else if(_sg_mc_comms_determinism && (!initial_global_state->send_deterministic && !initial_global_state->recv_deterministic)) {
        XBT_INFO("****************************************************");
        XBT_INFO("***** Non-deterministic communications pattern *****");
        XBT_INFO("****************************************************");
        XBT_INFO("%s", initial_global_state->send_diff);
        XBT_INFO("%s", initial_global_state->recv_diff);
        xbt_free(initial_global_state->send_diff);
        initial_global_state->send_diff = nullptr;
        xbt_free(initial_global_state->recv_diff);
        initial_global_state->recv_diff = nullptr;
        MC_print_statistics(mc_stats);
        mc_model_checker->exit(SIMGRID_MC_EXIT_NON_DETERMINISM);
      } 
    }
  }
    
  MC_comm_pattern_free(comm);

}

/********** Non Static functions ***********/

void MC_get_comm_pattern(xbt_dynar_t list, smx_simcall_t request, e_mc_call_type_t call_type, int backtracking)
{
  const smx_process_t issuer = MC_smx_simcall_get_issuer(request);
  mc_list_comm_pattern_t initial_pattern = xbt_dynar_get_as(
    initial_communications_pattern, issuer->pid, mc_list_comm_pattern_t);
  xbt_dynar_t incomplete_pattern = xbt_dynar_get_as(
    incomplete_communications_pattern, issuer->pid, xbt_dynar_t);

  mc_comm_pattern_t pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->data_size = -1;
  pattern->data = nullptr;
  pattern->index =
    initial_pattern->index_comm + xbt_dynar_length(incomplete_pattern);

  if (call_type == MC_CALL_TYPE_SEND) {
    /* Create comm pattern */
    pattern->type = SIMIX_COMM_SEND;
    pattern->comm_addr = simcall_comm_isend__get__result(request);

    s_smx_synchro_t synchro = mc_model_checker->process().read<s_smx_synchro_t>(
      (std::uint64_t) pattern->comm_addr);

    char* remote_name = mc_model_checker->process().read<char*>(
      (std::uint64_t)(synchro.comm.rdv ? &synchro.comm.rdv->name : &synchro.comm.rdv_cpy->name));
    pattern->rdv = mc_model_checker->process().read_string(remote_name);
    pattern->src_proc = MC_smx_resolve_process(synchro.comm.src_proc)->pid;
    pattern->src_host = MC_smx_process_get_host_name(issuer);

    struct s_smpi_mpi_request mpi_request =
      mc_model_checker->process().read<s_smpi_mpi_request>(
        (std::uint64_t) simcall_comm_isend__get__data(request));
    pattern->tag = mpi_request.tag;

    if(synchro.comm.src_buff != nullptr){
      pattern->data_size = synchro.comm.src_buff_size;
      pattern->data = xbt_malloc0(pattern->data_size);
      mc_model_checker->process().read_bytes(
        pattern->data, pattern->data_size, remote(synchro.comm.src_buff));
    }
    if(mpi_request.detached){
      if (!initial_global_state->initial_communications_pattern_done) {
        /* Store comm pattern */
        xbt_dynar_push(
          xbt_dynar_get_as(
            initial_communications_pattern, pattern->src_proc, mc_list_comm_pattern_t
          )->list,
          &pattern);
      } else {
        /* Evaluate comm determinism */
        deterministic_comm_pattern(pattern->src_proc, pattern, backtracking);
        xbt_dynar_get_as(
          initial_communications_pattern, pattern->src_proc, mc_list_comm_pattern_t
        )->index_comm++;
      }
      return;
    }
  } else if (call_type == MC_CALL_TYPE_RECV) {                      
    pattern->type = SIMIX_COMM_RECEIVE;
    pattern->comm_addr = simcall_comm_irecv__get__result(request);

    struct s_smpi_mpi_request mpi_request;
    mc_model_checker->process().read(
      &mpi_request, remote((struct s_smpi_mpi_request*)simcall_comm_irecv__get__data(request)));
    pattern->tag = mpi_request.tag;

    s_smx_synchro_t synchro;
    mc_model_checker->process().read(&synchro, remote(pattern->comm_addr));

    char* remote_name;
    mc_model_checker->process().read(&remote_name,
      remote(synchro.comm.rdv ? &synchro.comm.rdv->name : &synchro.comm.rdv_cpy->name));
    pattern->rdv = mc_model_checker->process().read_string(remote_name);
    pattern->dst_proc = MC_smx_resolve_process(synchro.comm.dst_proc)->pid;
    pattern->dst_host = MC_smx_process_get_host_name(issuer);
  } else
    xbt_die("Unexpected call_type %i", (int) call_type);

  xbt_dynar_push(
    xbt_dynar_get_as(incomplete_communications_pattern, issuer->pid, xbt_dynar_t),
    &pattern);

  XBT_DEBUG("Insert incomplete comm pattern %p for process %lu", pattern, issuer->pid);
}

void MC_complete_comm_pattern(xbt_dynar_t list, smx_synchro_t comm_addr, unsigned int issuer, int backtracking) {
  mc_comm_pattern_t current_comm_pattern;
  unsigned int cursor = 0;
  mc_comm_pattern_t comm_pattern;
  int completed = 0;

  /* Complete comm pattern */
  xbt_dynar_foreach(xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t), cursor, current_comm_pattern)
    if (current_comm_pattern->comm_addr == comm_addr) {
      update_comm_pattern(current_comm_pattern, comm_addr);
      completed = 1;
      xbt_dynar_remove_at(
        xbt_dynar_get_as(incomplete_communications_pattern, issuer, xbt_dynar_t),
        cursor, &comm_pattern);
      XBT_DEBUG("Remove incomplete comm pattern for process %u at cursor %u", issuer, cursor);
      break;
    }

  if(!completed)
    xbt_die("Corresponding communication not found!");

  mc_list_comm_pattern_t pattern = xbt_dynar_get_as(
    initial_communications_pattern, issuer, mc_list_comm_pattern_t);

  if (!initial_global_state->initial_communications_pattern_done)
    /* Store comm pattern */
    xbt_dynar_push(pattern->list, &comm_pattern);
  else {
    /* Evaluate comm determinism */
    deterministic_comm_pattern(issuer, comm_pattern, backtracking);
    pattern->index_comm++;
  }
}


/************************ Main algorithm ************************/

static int MC_modelcheck_comm_determinism_main(void);

static void MC_pre_modelcheck_comm_determinism(void)
{
  mc_state_t initial_state = nullptr;
  int i;
  const int maxpid = MC_smx_get_maxpid();

  if (_sg_mc_visited > 0)
    simgrid::mc::visited_states = simgrid::xbt::newDeleteDynar<simgrid::mc::VisitedState>();
 
  // Create initial_communications_pattern elements:
  initial_communications_pattern = xbt_dynar_new(sizeof(mc_list_comm_pattern_t), MC_list_comm_pattern_free_voidp);
  for (i=0; i < maxpid; i++){
    mc_list_comm_pattern_t process_list_pattern = xbt_new0(s_mc_list_comm_pattern_t, 1);
    process_list_pattern->list = xbt_dynar_new(sizeof(mc_comm_pattern_t), MC_comm_pattern_free_voidp);
    process_list_pattern->index_comm = 0;
    xbt_dynar_insert_at(initial_communications_pattern, i, &process_list_pattern);
  }

  // Create incomplete_communications_pattern elements:
  incomplete_communications_pattern = xbt_dynar_new(sizeof(xbt_dynar_t), xbt_dynar_free_voidp);
  for (i=0; i < maxpid; i++){
    xbt_dynar_t process_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), nullptr);
    xbt_dynar_insert_at(incomplete_communications_pattern, i, &process_pattern);
  }

  initial_state = MC_state_new();
  
  XBT_DEBUG("********* Start communication determinism verification *********");

  /* Wait for requests (schedules processes) */
  mc_model_checker->wait_for_requests();

  /* Get an enabled process and insert it in the interleave set of the initial state */
  for (auto& p : mc_model_checker->process().simix_processes())
    if (simgrid::mc::process_is_enabled(&p.copy))
      MC_state_interleave_process(initial_state, &p.copy);

  xbt_fifo_unshift(mc_stack, initial_state);
}

static int MC_modelcheck_comm_determinism_main(void)
{

  char *req_str = nullptr;
  int value;
  simgrid::mc::VisitedState* visited_state = nullptr;
  smx_simcall_t req = nullptr;
  mc_state_t state = nullptr, next_state = NULL;

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
        && (visited_state == nullptr)) {

      req_str = simgrid::mc::request_to_string(req, value, simgrid::mc::RequestType::simix);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);
      
      if (dot_output != nullptr)
        req_str = simgrid::mc::request_get_dot_output(req, value);

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = MC_get_call_type(req);

      /* Answer the request */
      simgrid::mc::handle_simcall(req, value);    /* After this call req is no longer useful */

      if(!initial_global_state->initial_communications_pattern_done)
        MC_handle_comm_pattern(call, req, value, initial_communications_pattern, 0);
      else
        MC_handle_comm_pattern(call, req, value, nullptr, 0);

      /* Wait for requests (schedules processes) */
      mc_model_checker->wait_for_requests();

      /* Create the new expanded state */
      next_state = MC_state_new();

      if ((visited_state = simgrid::mc::is_visited_state(next_state)) == nullptr) {

        /* Get enabled processes and insert them in the interleave set of the next state */
        for (auto& p : mc_model_checker->process().simix_processes())
          if (simgrid::mc::process_is_enabled(&p.copy))
            MC_state_interleave_process(next_state, &p.copy);

        if (dot_output != nullptr)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num,  next_state->num, req_str);

      } else if (dot_output != nullptr)
        fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n",
          state->num, visited_state->other_num == -1 ? visited_state->num : visited_state->other_num, req_str);

      xbt_fifo_unshift(mc_stack, next_state);

      if (dot_output != nullptr)
        xbt_free(req_str);

    } else {

      if (xbt_fifo_size(mc_stack) > _sg_mc_max_depth)
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      else if (visited_state != nullptr)
        XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.", visited_state->other_num == -1 ? visited_state->num : visited_state->other_num);
      else
        XBT_DEBUG("There are no more processes to interleave. (depth %d)", xbt_fifo_size(mc_stack));

      if (!initial_global_state->initial_communications_pattern_done) 
        initial_global_state->initial_communications_pattern_done = 1;

      /* Trash the current state, no longer needed */
      xbt_fifo_shift(mc_stack);
      MC_state_delete(state, !state->in_visited_states ? 1 : 0);
      XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);

      visited_state = nullptr;

      /* Check for deadlocks */
      if (mc_model_checker->checkDeadlock()) {
        MC_show_deadlock(nullptr);
        return SIMGRID_MC_EXIT_DEADLOCK;
      }

      while ((state = (mc_state_t) xbt_fifo_shift(mc_stack)) != nullptr)
        if (MC_state_interleave_size(state) && xbt_fifo_size(mc_stack) < _sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);
          xbt_fifo_unshift(mc_stack, state);

          MC_replay(mc_stack);

          XBT_DEBUG("Back-tracking to state %d at depth %d done", state->num, xbt_fifo_size(mc_stack));

          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);
          MC_state_delete(state, !state->in_visited_states ? 1 : 0);
        }

    }
  }

  MC_print_statistics(mc_stats);
  return SIMGRID_MC_EXIT_SUCCESS;
}

int MC_modelcheck_comm_determinism(void)
{
  XBT_INFO("Check communication determinism");
  simgrid::mc::reduction_mode = simgrid::mc::ReductionMode::none;
  mc_model_checker->wait_for_requests();

  if (mc_mode == MC_MODE_CLIENT)
    // This will move somehwere else:
    simgrid::mc::Client::get()->handleMessages();

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_pre_modelcheck_comm_determinism();

  initial_global_state = xbt_new0(s_mc_global_t, 1);
  initial_global_state->snapshot = simgrid::mc::take_snapshot(0);
  initial_global_state->initial_communications_pattern_done = 0;
  initial_global_state->recv_deterministic = 1;
  initial_global_state->send_deterministic = 1;
  initial_global_state->recv_diff = nullptr;
  initial_global_state->send_diff = nullptr;

  return MC_modelcheck_comm_determinism_main();
}

}
