/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <libgen.h>

#include "simgrid/sg_config.h"
#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
#include "xbt/fifo.h"
#include "mc_private.h"
#include "xbt/automaton.h"
#include "xbt/dict.h"

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

/* Configuration support */
e_mc_reduce_t mc_reduce_kind = e_mc_reduce_unset;

int _sg_do_model_check = 0;
int _sg_mc_checkpoint = 0;
int _sg_mc_sparse_checkpoint = 0;
int _sg_mc_soft_dirty = 1;
char *_sg_mc_property_file = NULL;
int _sg_mc_timeout = 0;
int _sg_mc_hash = 0;
int _sg_mc_max_depth = 1000;
int _sg_mc_visited = 0;
char *_sg_mc_dot_output_file = NULL;
int _sg_mc_comms_determinism = 0;
int _sg_mc_send_determinism = 0;
int _sg_mc_safety = 0;
int _sg_mc_liveness = 0;

int user_max_depth_reached = 0;

void _mc_cfg_cb_reduce(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a reduction strategy after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  char *val = xbt_cfg_get_string(_sg_cfg_set, name);
  if (!strcasecmp(val, "none")) {
    mc_reduce_kind = e_mc_reduce_none;
  } else if (!strcasecmp(val, "dpor")) {
    mc_reduce_kind = e_mc_reduce_dpor;
  } else {
    xbt_die("configuration option %s can only take 'none' or 'dpor' as a value",
            name);
  }
}

void _mc_cfg_cb_checkpoint(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_checkpoint = xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_sparse_checkpoint(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_sparse_checkpoint = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_soft_dirty(const char *name, int pos) {
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die("You are specifying a soft dirty value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_soft_dirty = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_property(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a property after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_property_file = xbt_cfg_get_string(_sg_cfg_set, name);
}

void _mc_cfg_cb_timeout(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a value to enable/disable timeout for wait requests after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_timeout = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_hash(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a value to enable/disable the use of global hash to speedup state comparaison, but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_hash = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

void _mc_cfg_cb_max_depth(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a max depth value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_max_depth = xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_visited(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a number of stored visited states after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_visited = xbt_cfg_get_int(_sg_cfg_set, name);
}

void _mc_cfg_cb_dot_output(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a file name for a dot output of graph state after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_dot_output_file = xbt_cfg_get_string(_sg_cfg_set, name);
}

void _mc_cfg_cb_comms_determinism(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a value to enable/disable the detection of determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_comms_determinism = xbt_cfg_get_boolean(_sg_cfg_set, name);
  mc_reduce_kind = e_mc_reduce_none;
}

void _mc_cfg_cb_send_determinism(const char *name, int pos)
{
  if (_sg_cfg_init_status && !_sg_do_model_check) {
    xbt_die
        ("You are specifying a value to enable/disable the detection of send-determinism in the communications schemes after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _sg_mc_send_determinism = xbt_cfg_get_boolean(_sg_cfg_set, name);
  mc_reduce_kind = e_mc_reduce_none;
}

/* MC global data structures */
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;
double *mc_time = NULL;
__thread mc_comparison_times_t mc_comp_times = NULL;
__thread double mc_snapshot_comparison_time;
mc_stats_t mc_stats = NULL;
mc_global_t initial_global_state = NULL;
xbt_fifo_t mc_stack = NULL;

/* Liveness */
xbt_automaton_t _mc_property_automaton = NULL;

/* Variables */
mc_object_info_t mc_libsimgrid_info = NULL;
mc_object_info_t mc_binary_info = NULL;

mc_object_info_t mc_object_infos[2] = { NULL, NULL };

size_t mc_object_infos_size = 2;

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

static void MC_init_debug_info(void)
{
  XBT_INFO("Get debug information ...");

  memory_map_t maps = MC_get_memory_map();

  /* Get local variables for state equality detection */
  mc_binary_info = MC_find_object_info(maps, xbt_binary_name, 1);
  mc_object_infos[0] = mc_binary_info;

  mc_libsimgrid_info = MC_find_object_info(maps, libsimgrid_path, 0);
  mc_object_infos[1] = mc_libsimgrid_info;

  // Use information of the other objects:
  MC_post_process_object_info(mc_binary_info);
  MC_post_process_object_info(mc_libsimgrid_info);

  MC_free_memory_map(maps);
  XBT_INFO("Get debug information done !");
}


mc_model_checker_t mc_model_checker = NULL;

void MC_init()
{
  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  MC_SET_MC_HEAP;

  mc_model_checker = xbt_new0(s_mc_model_checker_t, 1);
  mc_model_checker->pages = mc_pages_store_new();
  mc_model_checker->fd_clear_refs = -1;
  mc_model_checker->fd_pagemap = -1;

  mc_comp_times = xbt_new0(s_mc_comparison_times_t, 1);

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  MC_init_memory_map_info();
  MC_init_debug_info();         /* FIXME : get debug information only if liveness verification or visited state reduction */

  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0'))
    MC_init_dot_output();

  /* Init parmap */
  //parmap = xbt_parmap_mc_new(xbt_os_get_numcores(), XBT_PARMAP_DEFAULT);

  MC_SET_STD_HEAP;

  if (_sg_mc_visited > 0 || _sg_mc_liveness) {
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

    MC_ignore_global_variable("mc_model_checker");

    // Mot of those things could be moved into mc_model_checker:
    MC_ignore_global_variable("compared_pointers");
    MC_ignore_global_variable("mc_comp_times");
    MC_ignore_global_variable("mc_snapshot_comparison_time");
    MC_ignore_global_variable("mc_time");
    MC_ignore_global_variable("smpi_current_rank");
    MC_ignore_global_variable("counter");       /* Static variable used for tracing */
    MC_ignore_global_variable("maestro_stack_start");
    MC_ignore_global_variable("maestro_stack_end");
    MC_ignore_global_variable("smx_total_comms");
    MC_ignore_global_variable("communications_pattern");
    MC_ignore_global_variable("initial_communications_pattern");
    MC_ignore_global_variable("incomplete_communications_pattern");

    if (MC_is_active()) {
      MC_ignore_global_variable("mc_diff_info");
    }

    MC_ignore_heap(mc_time, simix_process_maxpid * sizeof(double));

    smx_process_t process;
    xbt_swag_foreach(process, simix_global->process_list) {
      MC_ignore_heap(&(process->process_hookup),
                     sizeof(process->process_hookup));
                     }
  }

  if (raw_mem_set)
    MC_SET_MC_HEAP;

}

/*******************************  Core of MC *******************************/
/**************************************************************************/

static void MC_modelcheck_comm_determinism_init(void)
{

  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_init();

  if (!mc_mem_set)
    MC_SET_MC_HEAP;

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_SET_STD_HEAP;

  MC_pre_modelcheck_comm_determinism();

  MC_SET_MC_HEAP;
  initial_global_state = xbt_new0(s_mc_global_t, 1);
  initial_global_state->snapshot = MC_take_snapshot(0);
  initial_global_state->initial_communications_pattern_done = 0;
  initial_global_state->comm_deterministic = 1;
  initial_global_state->send_deterministic = 1;
  MC_SET_STD_HEAP;

  MC_modelcheck_comm_determinism();

  if(mc_mem_set)
    MC_SET_MC_HEAP;

}

static void MC_modelcheck_safety_init(void)
{
  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  _sg_mc_safety = 1;

  MC_init();

  if (!mc_mem_set)
    MC_SET_MC_HEAP;

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

  if (mc_mem_set)
    MC_SET_MC_HEAP;

  xbt_abort();
  //MC_exit();
}

static void MC_modelcheck_liveness_init()
{

  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  _sg_mc_liveness = 1;

  MC_init();

  if (!mc_mem_set)
    MC_SET_MC_HEAP;

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  /* Create the initial state */
  initial_global_state = xbt_new0(s_mc_global_t, 1);

  MC_SET_STD_HEAP;

  MC_pre_modelcheck_liveness();

  /* We're done */
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);

  if (mc_mem_set)
    MC_SET_MC_HEAP;

}

void MC_do_the_modelcheck_for_real()
{

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    XBT_INFO("Check communication determinism");
    MC_modelcheck_comm_determinism_init();
  } else if (!_sg_mc_property_file || _sg_mc_property_file[0] == '\0') {
    if (mc_reduce_kind == e_mc_reduce_unset)
      mc_reduce_kind = e_mc_reduce_dpor;
    XBT_INFO("Check a safety property");
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

int SIMIX_pre_mc_random(smx_simcall_t simcall, int min, int max)
{

  return simcall->mc_value;
}


int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value; */
  return simcall_mc_random(min, max);
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !MC_request_is_visible(req))
        SIMIX_simcall_pre(req, 0);
    }
  }
}

int MC_deadlock_check()
{
  int deadlock = FALSE;
  smx_process_t process;
  if (xbt_swag_size(simix_global->process_list)) {
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list) {
      if (process->simcall.call != SIMCALL_NONE
          && MC_request_is_enabled(&process->simcall)) {
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack, int start)
{
  int raw_mem = (mmalloc_get_current_heap() == mc_heap);

  int value, i = 1, count = 1, call = 0, j;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;
  smx_process_t process = NULL;
  smx_action_t current_comm;

  XBT_DEBUG("**** Begin Replay ****");

  if (start == -1) {
    /* Restore the initial state */
    MC_restore_snapshot(initial_global_state->snapshot);
    /* At the moment of taking the snapshot the raw heap was set, so restoring
     * it will set it back again, we have to unset it to continue  */
    MC_SET_STD_HEAP;
  }

  start_item = xbt_fifo_get_last_item(stack);
  if (start != -1) {
    while (i != start) {
      start_item = xbt_fifo_get_prev_item(start_item);
      i++;
    }
  }

  MC_SET_MC_HEAP;

  if (mc_reduce_kind == e_mc_reduce_dpor) {
    xbt_dict_reset(first_enabled_state);
    xbt_swag_foreach(process, simix_global->process_list) {
      if (MC_process_is_enabled(process)) {
        char *key = bprintf("%lu", process->pid);
        char *data = bprintf("%d", count);
        xbt_dict_set(first_enabled_state, key, data, NULL);
        xbt_free(key);
      }
    }
  }

  if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
    for (j=0; j<simix_process_maxpid; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(communications_pattern, j, xbt_dynar_t));
    }
    for (j=0; j<simix_process_maxpid; j++) {
      xbt_dynar_reset((xbt_dynar_t)xbt_dynar_get_as(incomplete_communications_pattern, j, xbt_dynar_t));
    }
  }

  MC_SET_STD_HEAP;


  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);

    if (mc_reduce_kind == e_mc_reduce_dpor) {
      MC_SET_MC_HEAP;
      char *key = bprintf("%lu", saved_req->issuer->pid);
      if(xbt_dict_get_or_null(first_enabled_state, key))
         xbt_dict_remove(first_enabled_state, key);
      xbt_free(key);
      MC_SET_STD_HEAP;
    }

    if (saved_req) {
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->simcall;

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }

      /* TODO : handle test and testany simcalls */
      if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
        if (req->call == SIMCALL_COMM_ISEND)
          call = 1;
        else if (req->call == SIMCALL_COMM_IRECV)
          call = 2;
        else if (req->call == SIMCALL_COMM_WAIT)
          call = 3;
        else if (req->call == SIMCALL_COMM_WAITANY)
          call = 4;
      }

      SIMIX_simcall_pre(req, value);

      if (_sg_mc_comms_determinism || _sg_mc_send_determinism) {
        MC_SET_MC_HEAP;
        if (call == 1) { /* Send */
          get_comm_pattern(communications_pattern, req, call);
        } else if (call == 2) { /* Recv */
          get_comm_pattern(communications_pattern, req, call);
        } else if (call == 3) { /* Wait */
          current_comm = simcall_comm_wait__get__comm(req);
          if (current_comm->comm.refcount == 1)  /* First wait only must be considered */
            complete_comm_pattern(communications_pattern, current_comm);
        } else if (call == 4) { /* WaitAny */
          current_comm = xbt_dynar_get_as(simcall_comm_waitany__get__comms(req), value, smx_action_t);
          if (current_comm->comm.refcount == 1) /* First wait only must be considered */
            complete_comm_pattern(communications_pattern, current_comm);
        }
        MC_SET_STD_HEAP;
        call = 0;
      }


      MC_wait_for_requests();

      count++;

      if (mc_reduce_kind == e_mc_reduce_dpor) {
        MC_SET_MC_HEAP;
        /* Insert in dict all enabled processes */
        xbt_swag_foreach(process, simix_global->process_list) {
          if (MC_process_is_enabled(process) ) {
            char *key = bprintf("%lu", process->pid);
            if (xbt_dict_get_or_null(first_enabled_state, key) == NULL) {
              char *data = bprintf("%d", count);
              xbt_dict_set(first_enabled_state, key, data, NULL);
            }
            xbt_free(key);
          }
        }
        MC_SET_STD_HEAP;
      }
    }

    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

  }

  XBT_DEBUG("**** End Replay ****");

  if (raw_mem)
    MC_SET_MC_HEAP;
  else
    MC_SET_STD_HEAP;


}

void MC_replay_liveness(xbt_fifo_t stack, int all_stack)
{

  initial_global_state->raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  int value;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;
  mc_pair_t pair;
  int depth = 1;

  XBT_DEBUG("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_global_state->snapshot);

  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  if (!initial_global_state->raw_mem_set)
    MC_SET_STD_HEAP;

  if (all_stack) {

    item = xbt_fifo_get_last_item(stack);

    while (depth <= xbt_fifo_size(stack)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if (pair->requests > 0) {

        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);

        if (saved_req != NULL) {
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }

        }

        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;

      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;

      item = xbt_fifo_get_prev_item(item);
    }

  } else {

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if (pair->requests > 0) {

        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);

        if (saved_req != NULL) {
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);

          /* Debug information */
          if (XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)) {
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }

        }

        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;

      /* Update statistics */
      mc_stats->visited_pairs++;
      mc_stats->executed_transitions++;
    }
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

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_show_stack_safety(stack);

  if (!_sg_mc_checkpoint) {

    mc_state_t state;

    MC_SET_MC_HEAP;
    while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
      MC_state_delete(state);
    MC_SET_STD_HEAP;

  }

  if (raw_mem_set)
    MC_SET_MC_HEAP;
  else
    MC_SET_STD_HEAP;

}


void MC_show_stack_safety(xbt_fifo_t stack)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

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

  if (!raw_mem_set)
    MC_SET_STD_HEAP;
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


void MC_show_stack_liveness(xbt_fifo_t stack)
{
  int value;
  mc_pair_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;

  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pair_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if (req) {
      if (pair->requests > 0) {
        req_str = MC_request_to_string(req, value);
        XBT_INFO("%s", req_str);
        xbt_free(req_str);
      } else {
        XBT_INFO("End of system requests but evolution in Büchi automaton");
      }
    }
  }
}

void MC_dump_stack_liveness(xbt_fifo_t stack)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  mc_pair_t pair;

  MC_SET_MC_HEAP;
  while ((pair = (mc_pair_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
  MC_SET_STD_HEAP;

  if (raw_mem_set)
    MC_SET_MC_HEAP;

}


void MC_print_statistics(mc_stats_t stats)
{
  xbt_mheap_t previous_heap = mmalloc_get_current_heap();

  if (stats->expanded_pairs == 0) {
    XBT_INFO("Expanded states = %lu", stats->expanded_states);
    XBT_INFO("Visited states = %lu", stats->visited_states);
  } else {
    XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
    XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  }
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  MC_SET_MC_HEAP;
  if ((_sg_mc_dot_output_file != NULL) && (_sg_mc_dot_output_file[0] != '\0')) {
    fprintf(dot_output, "}\n");
    fclose(dot_output);
  }
  if (initial_global_state != NULL) {
    if (_sg_mc_comms_determinism)
      XBT_INFO("Communication-deterministic : %s",
               !initial_global_state->comm_deterministic ? "No" : "Yes");
    if (_sg_mc_send_determinism)
      XBT_INFO("Send-deterministic : %s",
               !initial_global_state->send_deterministic ? "No" : "Yes");
  }
  mmalloc_set_current_heap(previous_heap);
}

void MC_assert(int prop)
{
  if (MC_is_active() && !prop) {
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    MC_dump_stack_safety(mc_stack);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

void MC_cut(void)
{
  user_max_depth_reached = 1;
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

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

void MC_automaton_load(const char *file)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_load(_mc_property_automaton, file);

  MC_SET_STD_HEAP;

  if (raw_mem_set)
    MC_SET_MC_HEAP;

}

void MC_automaton_new_propositional_symbol(const char *id, void *fct)
{

  int raw_mem_set = (mmalloc_get_current_heap() == mc_heap);

  MC_SET_MC_HEAP;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_automaton_propositional_symbol_new(_mc_property_automaton, id, fct);

  MC_SET_STD_HEAP;

  if (raw_mem_set)
    MC_SET_MC_HEAP;

}
