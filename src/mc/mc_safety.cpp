/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <assert.h>

#include "mc_state.h"
#include "mc_request.h"
#include "mc_safety.h"
#include "mc_private.h"
#include "mc_record.h"
#include "mc_smx.h"
#include "mc_client.h"
#include "mc_exit.h"

#include "src/xbt/mmalloc/mmprivate.h"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_safety, mc,
                                "Logging specific to MC safety verification ");

}

static int is_exploration_stack_state(mc_state_t current_state){

  xbt_fifo_item_t item;
  mc_state_t stack_state;
  for(item = xbt_fifo_get_first_item(mc_stack); item != NULL; item = xbt_fifo_get_next_item(item)) {
    stack_state = (mc_state_t) xbt_fifo_get_item_content(item);
    if(snapshot_compare(stack_state, current_state) == 0){
      XBT_INFO("Non-progressive cycle : state %d -> state %d", stack_state->num, current_state->num);
      return 1;
    }
  }
  return 0;
}

static void MC_modelcheck_safety_init(void);

/**
 *  \brief Initialize the DPOR exploration algorithm
 */
static void MC_pre_modelcheck_safety()
{
  if (_sg_mc_visited > 0)
    visited_states = xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);

  mc_state_t initial_state = MC_state_new();

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  /* Get an enabled process and insert it in the interleave set of the initial state */
  smx_process_t process;
  MC_EACH_SIMIX_PROCESS(process,
    if (MC_process_is_enabled(process)) {
      MC_state_interleave_process(initial_state, process);
      if (mc_reduce_kind != e_mc_reduce_none)
        break;
    }
  );

  xbt_fifo_unshift(mc_stack, initial_state);
}


/** \brief Model-check the application using a DFS exploration
 *         with DPOR (Dynamic Partial Order Reductions)
 */
int MC_modelcheck_safety(void)
{
  MC_modelcheck_safety_init();

  char *req_str = NULL;
  int value;
  smx_simcall_t req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL;
  xbt_fifo_item_t item = NULL;
  mc_visited_state_t visited_state = NULL;

  while (xbt_fifo_size(mc_stack) > 0) {

    /* Get current state */
    state = (mc_state_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG
        ("Exploration depth=%d (state=%p, num %d)(%u interleave, user_max_depth %d)",
         xbt_fifo_size(mc_stack), state, state->num,
         MC_state_interleave_size(state), user_max_depth_reached);

    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack) <= _sg_mc_max_depth && !user_max_depth_reached
        && (req = MC_state_get_request(state, &value)) && visited_state == NULL) {

      req_str = MC_request_to_string(req, value, MC_REQUEST_SIMIX);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);

      if (dot_output != NULL) {
        req_str = MC_request_get_dot_output(req, value);
      }

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      // TODO, bundle both operations in a single message
      //   MC_execute_transition(req, value)

      /* Answer the request */
      MC_simcall_handle(req, value);
      MC_wait_for_requests();

      /* Create the new expanded state */
      next_state = MC_state_new();

      if(_sg_mc_termination && is_exploration_stack_state(next_state)){
          MC_show_non_termination();
          return SIMGRID_MC_EXIT_NON_TERMINATION;
      }

      if ((visited_state = is_visited_state(next_state)) == NULL) {

        /* Get an enabled process and insert it in the interleave set of the next state */
        smx_process_t process = NULL;
        MC_EACH_SIMIX_PROCESS(process,
          if (MC_process_is_enabled(process)) {
            MC_state_interleave_process(next_state, process);
            if (mc_reduce_kind != e_mc_reduce_none)
              break;
          }
        );

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, next_state->num, req_str);

      } else {

        if (dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, visited_state->other_num == -1 ? visited_state->num : visited_state->other_num, req_str);

      }

      xbt_fifo_unshift(mc_stack, next_state);

      if (dot_output != NULL)
        xbt_free(req_str);

      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {

      if ((xbt_fifo_size(mc_stack) > _sg_mc_max_depth) || user_max_depth_reached || visited_state != NULL) {

        if (user_max_depth_reached && visited_state == NULL)
          XBT_DEBUG("User max depth reached !");
        else if (visited_state == NULL)
          XBT_WARN("/!\\ Max depth reached ! /!\\ ");
        else
          XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.", visited_state->other_num == -1 ? visited_state->num : visited_state->other_num);

      } else {

        XBT_DEBUG("There are no more processes to interleave. (depth %d)", xbt_fifo_size(mc_stack) + 1);

      }

      /* Trash the current state, no longer needed */
      xbt_fifo_shift(mc_stack);
      XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack) + 1);
      MC_state_delete(state, !state->in_visited_states ? 1 : 0);

      visited_state = NULL;

      /* Check for deadlocks */
      if (MC_deadlock_check()) {
        MC_show_deadlock(NULL);
        return SIMGRID_MC_EXIT_DEADLOCK;
      }

      /* Traverse the stack backwards until a state with a non empty interleave
         set is found, deleting all the states that have it empty in the way.
         For each deleted state, check if the request that has generated it 
         (from it's predecesor state), depends on any other previous request 
         executed before it. If it does then add it to the interleave set of the
         state that executed that previous request. */

      while ((state = (mc_state_t) xbt_fifo_shift(mc_stack))) {
        if (mc_reduce_kind == e_mc_reduce_dpor) {
          req = MC_state_get_internal_request(state);
          if (req->call == SIMCALL_MUTEX_LOCK || req->call == SIMCALL_MUTEX_TRYLOCK)
            xbt_die("Mutex is currently not supported with DPOR, "
              "use --cfg=model-check/reduction:none");
          const smx_process_t issuer = MC_smx_simcall_get_issuer(req);
          xbt_fifo_foreach(mc_stack, item, prev_state, mc_state_t) {
            if (MC_request_depend(req, MC_state_get_internal_request(prev_state))) {
              if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
                XBT_DEBUG("Dependent Transitions:");
                smx_simcall_t prev_req = MC_state_get_executed_request(prev_state, &value);
                req_str = MC_request_to_string(prev_req, value, MC_REQUEST_INTERNAL);
                XBT_DEBUG("%s (state=%d)", req_str, prev_state->num);
                xbt_free(req_str);
                prev_req = MC_state_get_executed_request(state, &value);
                req_str = MC_request_to_string(prev_req, value, MC_REQUEST_EXECUTED);
                XBT_DEBUG("%s (state=%d)", req_str, state->num);
                xbt_free(req_str);
              }

              if (!MC_state_process_is_done(prev_state, issuer))
                MC_state_interleave_process(prev_state, issuer);
              else
                XBT_DEBUG("Process %p is in done set", req->issuer);

              break;

            } else if (req->issuer == MC_state_get_internal_request(prev_state)->issuer) {

              XBT_DEBUG("Simcall %d and %d with same issuer", req->call, MC_state_get_internal_request(prev_state)->call);
              break;

            } else {

              const smx_process_t previous_issuer = MC_smx_simcall_get_issuer(MC_state_get_internal_request(prev_state));
              XBT_DEBUG("Simcall %d, process %lu (state %d) and simcall %d, process %lu (state %d) are independant",
                        req->call, issuer->pid, state->num,
                        MC_state_get_internal_request(prev_state)->call,
                        previous_issuer->pid,
                        prev_state->num);

            }
          }
        }

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
  }

  XBT_INFO("No property violation found.");
  MC_print_statistics(mc_stats);
  return SIMGRID_MC_EXIT_SUCCESS;
}

static void MC_modelcheck_safety_init(void)
{
  if(_sg_mc_termination)
    mc_reduce_kind = e_mc_reduce_none;
  else if (mc_reduce_kind == e_mc_reduce_unset)
    mc_reduce_kind = e_mc_reduce_dpor;
  _sg_mc_safety = 1;
  if (_sg_mc_termination)
    XBT_INFO("Check non progressive cycles");
  else
    XBT_INFO("Check a safety property");
  MC_wait_for_requests();

  XBT_DEBUG("Starting the safety algorithm");

  _sg_mc_safety = 1;

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_pre_modelcheck_safety();

  /* Save the initial state */
  initial_global_state = xbt_new0(s_mc_global_t, 1);
  initial_global_state->snapshot = MC_take_snapshot(0);
}
