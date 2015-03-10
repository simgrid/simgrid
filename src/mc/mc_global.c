/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string.h>

#include "mc_base.h"

#ifndef _XBT_WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include "simgrid/sg_config.h"
#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "xbt/fifo.h"
#include "xbt/automaton.h"
#include "xbt/dict.h"

#ifdef HAVE_MC
#include <libunwind.h>
#include <xbt/mmalloc.h>

#include "../xbt/mmalloc/mmprivate.h"
#include "mc_object_info.h"
#include "mc_comm_pattern.h"
#include "mc_request.h"
#include "mc_safety.h"
#include "mc_memory_map.h"
#include "mc_snapshot.h"
#include "mc_liveness.h"
#include "mc_private.h"
#include "mc_unw.h"
#include "mc_smx.h"
#endif
#include "mc_record.h"
#include "mc_protocol.h"
#include "mc_client.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

e_mc_mode_t mc_mode;

double *mc_time = NULL;

#ifdef HAVE_MC
int user_max_depth_reached = 0;

/* MC global data structures */
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;

__thread mc_comparison_times_t mc_comp_times = NULL;
__thread double mc_snapshot_comparison_time;
mc_stats_t mc_stats = NULL;
mc_global_t initial_global_state = NULL;
xbt_fifo_t mc_stack = NULL;

/* Liveness */
xbt_automaton_t _mc_property_automaton = NULL;

/* Dot output */
FILE *dot_output = NULL;
const char *colors[13];


/*******************************  Initialisation of MC *******************************/
/*********************************************************************************/

static void MC_init_dot_output()
{                               /* FIXME : more colors */

  colors[0] = "blue";
  colors[1] = "red";
  colors[2] = "green3";
  colors[3] = "goldenrod";
  colors[4] = "brown";
  colors[5] = "purple";
  colors[6] = "magenta";
  colors[7] = "turquoise4";
  colors[8] = "gray25";
  colors[9] = "forestgreen";
  colors[10] = "hotpink";
  colors[11] = "lightblue";
  colors[12] = "tan";

  dot_output = fopen(_sg_mc_dot_output_file, "w");

  if (dot_output == NULL) {
    perror("Error open dot output file");
    xbt_abort();
  }

  fprintf(dot_output,
          "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node [fontsize=10, shape=circle,width=.5 ]; graph [resolution=20, fontsize=10];\n");

}

void MC_init()
{
  MC_init_pid(getpid(), -1);
}

void MC_init_pid(pid_t pid, int socket)
{
  if (mc_mode == MC_MODE_NONE) {
    if (getenv(MC_ENV_SOCKET_FD)) {
      mc_mode = MC_MODE_CLIENT;
    } else {
      mc_mode = MC_MODE_STANDALONE;
    }
  }

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  mc_model_checker = MC_model_checker_new(pid, socket);

  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0'))
    MC_init_dot_output();

  /* Init parmap */
  //parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  MC_SET_STD_HEAP;

  if (_sg_mc_visited > 0 || _sg_mc_liveness  || _sg_mc_termination) {
    /* Ignore some variables from xbt/ex.h used by exception e for stacks comparison */
    MC_ignore_local_variable("e", "*");
    MC_ignore_local_variable("__ex_cleanup", "*");
    MC_ignore_local_variable("__ex_mctx_en", "*");
    MC_ignore_local_variable("__ex_mctx_me", "*");
    MC_ignore_local_variable("__xbt_ex_ctx_ptr", "*");
    MC_ignore_local_variable("_log_ev", "*");
    MC_ignore_local_variable("_throw_ctx", "*");
    MC_ignore_local_variable("ctx", "*");

    MC_ignore_local_variable("self", "simcall_BODY_mc_snapshot");
    MC_ignore_local_variable("next_cont"
      "ext", "smx_ctx_sysv_suspend_serial");
    MC_ignore_local_variable("i", "smx_ctx_sysv_suspend_serial");

    /* Ignore local variable about time used for tracing */
    MC_ignore_local_variable("start_time", "*");

    /* Main MC state: */
    MC_ignore_global_variable("mc_model_checker");
    MC_ignore_global_variable("initial_communications_pattern");
    MC_ignore_global_variable("incomplete_communications_pattern");
    MC_ignore_global_variable("nb_comm_pattern");

    /* MC __thread variables: */
    MC_ignore_global_variable("mc_diff_info");
    MC_ignore_global_variable("mc_comp_times");
    MC_ignore_global_variable("mc_snapshot_comparison_time");

    /* This MC state is used in MC replay as well: */
    MC_ignore_global_variable("mc_time");

    /* Static variable used for tracing */
    MC_ignore_global_variable("counter");

    /* SIMIX */
    MC_ignore_global_variable("smx_total_comms");

    if (mc_mode == MC_MODE_STANDALONE || mc_mode == MC_MODE_CLIENT) {
      /* Those requests are handled on the client side and propagated by message
       * to the server: */

      MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));

      smx_process_t process;
      xbt_swag_foreach(process, simix_global->process_list) {
        MC_ignore_heap(&(process->process_hookup), sizeof(process->process_hookup));
      }
    }
  }

  mmalloc_set_current_heap(heap);

  if (mc_mode == MC_MODE_CLIENT) {
    // This will move somehwere else:
    MC_client_handle_messages();
  }

}

/*******************************  Core of MC *******************************/
/**************************************************************************/

static void MC_modelcheck_comm_determinism_init(void)
{
  MC_init();

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_SET_STD_HEAP;

  MC_pre_modelcheck_comm_determinism();

  MC_SET_MC_HEAP;
  initial_global_state = xbt_new0(s_mc_global_t, 1);
  initial_global_state->snapshot = MC_take_snapshot(0);
  initial_global_state->initial_communications_pattern_done = 0;
  initial_global_state->recv_deterministic = 1;
  initial_global_state->send_deterministic = 1;
  initial_global_state->recv_diff = NULL;
  initial_global_state->send_diff = NULL;

  MC_SET_STD_HEAP;

  MC_modelcheck_comm_determinism();

  mmalloc_set_current_heap(heap);
}

static void MC_modelcheck_safety_init(void)
{
  _sg_mc_safety = 1;

  MC_init();

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_SET_STD_HEAP;

  MC_pre_modelcheck_safety();

  MC_SET_MC_HEAP;
  /* Save the initial state */
  initial_global_state = xbt_new0(s_mc_global_t, 1);
  initial_global_state->snapshot = MC_take_snapshot(0);
  MC_SET_STD_HEAP;

  MC_modelcheck_safety();

  mmalloc_set_current_heap(heap);

  xbt_abort();
  //MC_exit();
}

static void MC_modelcheck_liveness_init()
{
  _sg_mc_liveness = 1;

  MC_init();

  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  /* Create the initial state */
  initial_global_state = xbt_new0(s_mc_global_t, 1);

  MC_SET_STD_HEAP;

  MC_pre_modelcheck_liveness();

  /* We're done */
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);

  mmalloc_set_current_heap(heap);

}

void MC_do_the_modelcheck_for_real()
{

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    XBT_INFO("Check communication determinism");
    mc_reduce_kind = e_mc_reduce_none;
    MC_modelcheck_comm_determinism_init();
  } else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0') {
    if(_sg_mc_termination){
      XBT_INFO("Check non progressive cycles");
      mc_reduce_kind = e_mc_reduce_none;
    }else{
      XBT_INFO("Check a safety property");
    }
    MC_modelcheck_safety_init();
  } else {
    if (mc_reduce_kind == e_mc_reduce_unset)
      mc_reduce_kind = e_mc_reduce_none;
    XBT_INFO("Check the liveness property %s", _sg_mc_property_file);
    MC_automaton_load(_sg_mc_property_file);
    MC_modelcheck_liveness_init();
  }
}


void MC_exit(void)
{
  xbt_free(mc_time);

  MC_memory_exit();
  //xbt_abort();
}

int MC_deadlock_check()
{
  if (mc_mode == MC_MODE_SERVER) {
    int res;
    if ((res = MC_protocol_send_simple_message(mc_model_checker->process.socket,
      MC_MESSAGE_DEADLOCK_CHECK)))
      xbt_die("Could not check deadlock state");
    s_mc_int_message_t message;
    ssize_t s = MC_receive_message(mc_model_checker->process.socket, &message, sizeof(message));
    if (s == -1)
      xbt_die("Could not receive message");
    else if (s != sizeof(message) || message.type != MC_MESSAGE_DEADLOCK_CHECK_REPLY) {
      xbt_die("Unexpected message, expected MC_MESSAGE_DEADLOCK_CHECK_REPLY %i %i vs %i %i",
        (int) s, (int) message.type, (int) sizeof(message), (int) MC_MESSAGE_DEADLOCK_CHECK_REPLY
        );
    }
    else
      return message.value;
  }

  int deadlock = FALSE;
  smx_process_t process;
  if (xbt_swag_size(simix_global->process_list)) {
    deadlock = TRUE;
    MC_EACH_SIMIX_PROCESS(process,
      if (MC_process_is_enabled(process)) {
        deadlock = FALSE;
        break;
      }
    );
  }
  return deadlock;
}

void handle_comm_pattern(e_mc_call_type_t call_type, smx_simcall_t req, int value, xbt_dynar_t pattern, int backtracking) {

  switch(call_type) {
  case MC_CALL_TYPE_NONE:
    break;
  case MC_CALL_TYPE_SEND:
  case MC_CALL_TYPE_RECV:
    get_comm_pattern(pattern, req, call_type, backtracking);
    break;
  case MC_CALL_TYPE_WAIT:
  case MC_CALL_TYPE_WAITANY:
    {
      smx_synchro_t current_comm = NULL;
      if (call_type == MC_CALL_TYPE_WAIT)
        current_comm = simcall_comm_wait__get__comm(req);
      else
        current_comm = xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value, smx_synchro_t);
      complete_comm_pattern(pattern, current_comm, req->issuer->pid, backtracking);
    }
    break;
  default:
    xbt_die("Unexpected call type %i", (int)call_type);
  }
  
}

static void MC_restore_communications_pattern(mc_state_t state) {
  mc_list_comm_pattern_t list_process_comm;
  unsigned int cursor, cursor2;
  xbt_dynar_foreach(initial_communications_pattern, cursor, list_process_comm){
    list_process_comm->index_comm = (int)xbt_dynar_get_as(state->index_comm, cursor, int);
  }
  mc_comm_pattern_t comm;
  cursor = 0;
  xbt_dynar_t initial_incomplete_process_comms, incomplete_process_comms;
  for(int i=0; i<simix_process_maxpid; i++){
    initial_incomplete_process_comms = xbt_dynar_get_as(incomplete_communications_pattern, i, xbt_dynar_t);
    xbt_dynar_reset(initial_incomplete_process_comms);
    incomplete_process_comms = xbt_dynar_get_as(state->incomplete_comm_pattern, i, xbt_dynar_t);
    xbt_dynar_foreach(incomplete_process_comms, cursor2, comm) {
      mc_comm_pattern_t copy_comm = xbt_new0(s_mc_comm_pattern_t, 1);
      copy_comm->index = comm->index;
      copy_comm->type = comm->type;
      copy_comm->comm = comm->comm;
      copy_comm->rdv = strdup(comm->rdv);
      copy_comm->data_size = -1;
      copy_comm->data = NULL;
      if(comm->type == SIMIX_COMM_SEND){
        copy_comm->src_proc = comm->src_proc;
        copy_comm->src_host = comm->src_host;
        if(comm->data != NULL){
          copy_comm->data_size = comm->data_size;
          copy_comm->data = xbt_malloc0(comm->data_size);
          memcpy(copy_comm->data, comm->data, comm->data_size);
        }
      }else{
        copy_comm->dst_proc = comm->dst_proc;
        copy_comm->dst_host = comm->dst_host;
      }
      xbt_dynar_push(initial_incomplete_process_comms, &copy_comm);
    }
  }
}

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  int value, count = 1, j;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;
  
  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0 || _sg_mc_termination || _sg_mc_visited > 0) {
    start_item = xbt_fifo_get_first_item(stack);
    state = (mc_state_t)xbt_fifo_get_item_content(start_item);
    if(state->system_state){
      MC_restore_snapshot(state->system_state);
      if(_sg_mc_comms_determinism || _sg_mc_send_determinism) 
        MC_restore_communications_pattern(state);
      MC_SET_STD_HEAP;
      return;
    }
  }


  /* Restore the initial state */
  MC_restore_snapshot(initial_global_state->snapshot);
  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  MC_SET_STD_HEAP;

  start_item = xbt_fifo_get_last_item(stack);
  
  MC_SET_MC_HEAP;

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    for (j=0; j<simix_process_maxpid; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
      ((mc_list_comm_pattern_t)xbt_dynar_get_as(initial_communications_pattern, j, mc_list_comm_pattern_t))->index_comm = 0;
    }
  }

  MC_SET_STD_HEAP;

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
    
    if (saved_req) {
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */

      const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
      req = &issuer->simcall;

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }

      /* TODO : handle test and testany simcalls */
      e_mc_call_type_t call = MC_CALL_TYPE_NONE;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        call = mc_get_call_type(req);

      MC_simcall_handle(req, value);

      MC_SET_MC_HEAP;
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism)
        handle_comm_pattern(call, req, value, NULL, 1);
      MC_SET_STD_HEAP;
      
      MC_wait_for_requests();

      count++;
    }

    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");
  mmalloc_set_current_heap(heap);
}

void MC_replay_liveness(xbt_fifo_t stack)
{

  initial_global_state->raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  xbt_fifo_item_t item;
  mc_pair_t pair = NULL;
  mc_state_t state = NULL;
  smx_simcall_t req = NULL, saved_req = NULL;
  int value, depth = 1;
  char *req_str;

  XBT_DEBUG("**** Begin Replay ****");

  /* Intermediate backtracking */
  if(_sg_mc_checkpoint > 0) {
    item = xbt_fifo_get_first_item(stack);
    pair = (mc_pair_t) xbt_fifo_get_item_content(item);
    if(pair->graph_state->system_state){
      MC_restore_snapshot(pair->graph_state->system_state);
      MC_SET_STD_HEAP;
      return;
    }
  }

  /* Restore the initial state */
  MC_restore_snapshot(initial_global_state->snapshot);

  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  if (!initial_global_state->raw_mem_set)
    MC_SET_STD_HEAP;

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);

      state = (mc_state_t) pair->graph_state;

      if (pair->exploration_started) {

        saved_req = MC_state_get_executed_request(state, &value);

        if (saved_req != NULL) {
          /* because we got a copy of the executed request, we have to fetch the
             real one, pointed by the request field of the issuer process */
          const smx_process_t issuer = MC_smx_simcall_get_issuer(saved_req);
          req = &issuer->simcall;

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }

        }

        MC_simcall_handle(req, value);
        MC_wait_for_requests();
      }

      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      depth++;
      
    }

  XBT_DEBUG("**** End Replay ****");

  if (initial_global_state->raw_mem_set)
    MC_SET_MC_HEAP;
  else
    MC_SET_STD_HEAP;

}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
 */
void MC_dump_stack_safety(xbt_fifo_t stack)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  MC_show_stack_safety(stack);
  
  mc_state_t state;
  
  MC_SET_MC_HEAP;
  while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
    MC_state_delete(state, !state->in_visited_states ? 1 : 0);
  mmalloc_set_current_heap(heap);
}


void MC_show_stack_safety(xbt_fifo_t stack)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;

  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state, &value);
    if (req) {
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }

  mmalloc_set_current_heap(heap);
}

void MC_show_deadlock(smx_simcall_t req)
{
  /*char *req_str = NULL; */
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
     XBT_INFO("%s", req_str);
     xbt_free(req_str); */
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack);
  MC_print_statistics(mc_stats);
}

void MC_show_non_termination(void){
  XBT_INFO("******************************************");
  XBT_INFO("*** NON-PROGRESSIVE CYCLE DETECTED ***");
  XBT_INFO("******************************************");
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack);
  MC_print_statistics(mc_stats);
}


void MC_show_stack_liveness(xbt_fifo_t stack)
{
  int value;
  mc_pair_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;

  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pair_t) (xbt_fifo_get_item_content(item))) : (NULL));
       item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req && req->call != SIMCALL_NONE) {
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}


void MC_dump_stack_liveness(xbt_fifo_t stack)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
  mc_pair_t pair;
  while ((pair = (mc_pair_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
  mmalloc_set_current_heap(heap);
}

void MC_print_statistics(mc_stats_t stats)
{
  if(_sg_mc_comms_determinism) {
    if (!initial_global_state->recv_deterministic && initial_global_state->send_deterministic){
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-send-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", initial_global_state->recv_diff);
    }else if(!initial_global_state->send_deterministic && initial_global_state->recv_deterministic) {
      XBT_INFO("******************************************************");
      XBT_INFO("**** Only-recv-deterministic communication pattern ****");
      XBT_INFO("******************************************************");
      XBT_INFO("%s", initial_global_state->send_diff);
    }
  }

  if (stats->expanded_pairs == 0) {
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  } else {
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0')) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (initial_global_state != NULL && (_sg_mc_comms_determinism || _sg_mc_send_determinism)) {
    XBT_INFO("Send-deterministic : %s", !initial_global_state->send_deterministic ? "No" : "Yes");
    if (_sg_mc_comms_determinism)
      XBT_INFO("Recv-deterministic : %s", !initial_global_state->recv_deterministic ? "No" : "Yes");
  }
  mmalloc_set_current_heap(heap);
}

void MC_automaton_load(const char *file)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_load(_mc_property_automaton, file);
  mmalloc_set_current_heap(heap);
}

static void register_symbol(xbt_automaton_propositional_symbol_t symbol)
{
  if (mc_mode != MC_MODE_CLIENT)
    return;
  s_mc_register_symbol_message_t message;
  message.type = MC_MESSAGE_REGISTER_SYMBOL;
  const char* name = xbt_automaton_propositional_symbol_get_name(symbol);
  if (strlen(name) + 1 > sizeof(message.name))
    xbt_die("Symbol is too long");
  strncpy(message.name, name, sizeof(message.name));
  message.callback = xbt_automaton_propositional_symbol_get_callback(symbol);
  message.data = xbt_automaton_propositional_symbol_get_data(symbol);
  MC_client_send_message(&message, sizeof(message));
}

void MC_automaton_new_propositional_symbol(const char *id, int(*fct)(void))
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_propositional_symbol_t symbol = xbt_automaton_propositional_symbol_new(_mc_property_automaton, id, fct);
  register_symbol(symbol);
  mmalloc_set_current_heap(heap);
}

void MC_automaton_new_propositional_symbol_pointer(const char *id, int* value)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();
  xbt_automaton_propositional_symbol_t symbol = xbt_automaton_propositional_symbol_new_pointer(_mc_property_automaton, id, value);
  register_symbol(symbol);
  mmalloc_set_current_heap(heap);
}

void MC_automaton_new_propositional_symbol_callback(const char* id,
  xbt_automaton_propositional_symbol_callback_type callback,
  void* data, xbt_automaton_propositional_symbol_free_function_type free_function)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();
  xbt_automaton_propositional_symbol_t symbol = xbt_automaton_propositional_symbol_new_callback(
    _mc_property_automaton, id, callback, data, free_function);
  register_symbol(symbol);
  mmalloc_set_current_heap(heap);
}

void MC_dump_stacks(FILE* file)
{
  xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);

  int nstack = 0;
  stack_region_t current_stack;
  unsigned cursor;
  xbt_dynar_foreach(stacks_areas, cursor, current_stack) {
    unw_context_t * context = (unw_context_t *)current_stack->context;
    fprintf(file, "Stack %i:\n", nstack);

    int nframe = 0;
    char buffer[100];
    unw_cursor_t c;
    unw_init_local (&c, context);
    unw_word_t off;
    do {
      const char * name = !unw_get_proc_name(&c, buffer, 100, &off) ? buffer : "?";
      fprintf(file, "  %i: %s\n", nframe, name);
      ++nframe;
    } while(unw_step(&c));

    ++nstack;
  }
  mmalloc_set_current_heap(heap);
}
#endif

double MC_process_clock_get(smx_process_t process)
{
  if (mc_time) {
    if (process != NULL)
      return mc_time[process->pid];
    else
      return -1;
  } else {
    return 0;
  }
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}
