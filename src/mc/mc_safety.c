/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

#include "xbt/mmalloc/mmprivate.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_safety, mc,
                                "Logging specific to MC safety verification ");

xbt_dict_t first_enabled_state;

/**
 *  \brief Initialize the DPOR exploration algorithm
 */
void MC_pre_modelcheck_safety()
{

  int mc_mem_set = (mmalloc_get_current_heap() == mc_heap);

  mc_state_t initial_state = NULL;
  smx_process_t process;

  /* Create the initial state and push it into the exploration stack */
  if (!mc_mem_set)
    MC_SET_MC_HEAP;

  if (_sg_mc_visited > 0)
    visited_states =
        xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);

  if (mc_reduce_kind == e_mc_reduce_dpor)
    first_enabled_state = xbt_dict_new_homogeneous(&xbt_free_f);

  initial_state = MC_state_new();

  MC_SET_STD_HEAP;

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_SET_MC_HEAP;

  /* Get an enabled process and insert it in the interleave set of the initial state */
  xbt_swag_foreach(process, simix_global->process_list) {
    if (MC_process_is_enabled(process)) {
      MC_state_interleave_process(initial_state, process);
      if (mc_reduce_kind != e_mc_reduce_none)
        break;
    }
  }

  xbt_fifo_unshift(mc_stack, initial_state);

  if (mc_reduce_kind == e_mc_reduce_dpor) {
    /* To ensure the soundness of DPOR, we have to keep a list of 
       processes which are still enabled at each step of the exploration. 
       If max depth is reached, we interleave them in the state in which they have 
       been enabled for the first time. */
    xbt_swag_foreach(process, simix_global->process_list) {
      if (MC_process_is_enabled(process)) {
        char *key = bprintf("%lu", process->pid);
        char *data = bprintf("%d", xbt_fifo_size(mc_stack));
        xbt_dict_set(first_enabled_state, key, data, NULL);
        xbt_free(key);
      }
    }
  }

  if (!mc_mem_set)
    MC_SET_STD_HEAP;
}


/** \brief Model-check the application using a DFS exploration
 *         with DPOR (Dynamic Partial Order Reductions)
 */
void MC_modelcheck_safety(void)
{

  char *req_str = NULL;
  int value;
  smx_simcall_t req = NULL, prev_req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state =
      NULL, restored_state = NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  mc_state_t state_test = NULL;
  int pos;
  mc_visited_state_t visited_state = NULL;
  int enabled = 0;

  while (xbt_fifo_size(mc_stack) > 0) {

    /* Get current state */
    state = (mc_state_t)
        xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG
        ("Exploration depth=%d (state=%p, num %d)(%u interleave, user_max_depth %d, first_enabled_state size : %d)",
         xbt_fifo_size(mc_stack), state, state->num,
         MC_state_interleave_size(state), user_max_depth_reached,
         xbt_dict_size(first_enabled_state));

    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack) <= _sg_mc_max_depth && !user_max_depth_reached
        && (req = MC_state_get_request(state, &value)) && visited_state == NULL) {

      /* Debug information */
      if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
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

      if (mc_reduce_kind == e_mc_reduce_dpor) {
        MC_SET_MC_HEAP;
        char *key = bprintf("%lu", req->issuer->pid);
        xbt_dict_remove(first_enabled_state, key);
        xbt_free(key);
        MC_SET_STD_HEAP;
      }

      /* Answer the request */
      SIMIX_simcall_pre(req, value);

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_MC_HEAP;

      next_state = MC_state_new();

      if ((visited_state = is_visited_state()) == NULL) {

        /* Get an enabled process and insert it in the interleave set of the next state */
        xbt_swag_foreach(process, simix_global->process_list) {
          if (MC_process_is_enabled(process)) {
            MC_state_interleave_process(next_state, process);
            if (mc_reduce_kind != e_mc_reduce_none)
              break;
          }
        }

        if (_sg_mc_checkpoint
            && ((xbt_fifo_size(mc_stack) + 1) % _sg_mc_checkpoint == 0)) {
          next_state->system_state = MC_take_snapshot(next_state->num);
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

      if (mc_reduce_kind == e_mc_reduce_dpor) {
        /* Insert in dict all enabled processes, if not included yet */
        xbt_swag_foreach(process, simix_global->process_list) {
          if (MC_process_is_enabled(process)) {
            char *key = bprintf("%lu", process->pid);
            if (xbt_dict_get_or_null(first_enabled_state, key) == NULL) {
              char *data = bprintf("%d", xbt_fifo_size(mc_stack));
              xbt_dict_set(first_enabled_state, key, data, NULL);
            }
            xbt_free(key);
          }
        }
      }

      if (dot_output != NULL)
        xbt_free(req_str);

      MC_SET_STD_HEAP;

      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {

      if ((xbt_fifo_size(mc_stack) > _sg_mc_max_depth) || user_max_depth_reached
          || visited_state != NULL) {

        if (user_max_depth_reached && visited_state == NULL)
          XBT_DEBUG("User max depth reached !");
        else if (visited_state == NULL)
          XBT_WARN("/!\\ Max depth reached ! /!\\ ");
        else
          XBT_DEBUG("State already visited (equal to state %d), exploration stopped on this path.", visited_state->other_num == -1 ? visited_state->num : visited_state->other_num);

        if (mc_reduce_kind == e_mc_reduce_dpor) {
          /* Interleave enabled processes in the state in which they have been enabled for the first time */
          xbt_swag_foreach(process, simix_global->process_list) {
            if (MC_process_is_enabled(process)) {
              char *key = bprintf("%lu", process->pid);
              enabled =
                  (int) strtoul(xbt_dict_get_or_null(first_enabled_state, key),
                                0, 10);
              xbt_free(key);
              int cursor = xbt_fifo_size(mc_stack);
              xbt_fifo_foreach(mc_stack, item, state_test, mc_state_t) {
                if (cursor-- == enabled) {
                  if (!MC_state_process_is_done(state_test, process)
                      && state_test->num != state->num) {
                    XBT_DEBUG("Interleave process %lu in state %d",
                              process->pid, state_test->num);
                    MC_state_interleave_process(state_test, process);
                    break;
                  }
                }
              }
            }
          }
        }

      } else {

        XBT_DEBUG("There are no more processes to interleave. (depth %d)",
                  xbt_fifo_size(mc_stack) + 1);

      }

      MC_SET_MC_HEAP;

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
      /* Traverse the stack backwards until a state with a non empty interleave
         set is found, deleting all the states that have it empty in the way.
         For each deleted state, check if the request that has generated it 
         (from it's predecesor state), depends on any other previous request 
         executed before it. If it does then add it to the interleave set of the
         state that executed that previous request. */

      while ((state = xbt_fifo_shift(mc_stack)) != NULL) {
        if (mc_reduce_kind == e_mc_reduce_dpor) {
          req = MC_state_get_internal_request(state);
          xbt_fifo_foreach(mc_stack, item, prev_state, mc_state_t) {
            if (MC_request_depend
                (req, MC_state_get_internal_request(prev_state))) {
              if (XBT_LOG_ISENABLED(mc_safety, xbt_log_priority_debug)) {
                XBT_DEBUG("Dependent Transitions:");
                prev_req = MC_state_get_executed_request(prev_state, &value);
                req_str = MC_request_to_string(prev_req, value);
                XBT_DEBUG("%s (state=%d)", req_str, prev_state->num);
                xbt_free(req_str);
                prev_req = MC_state_get_executed_request(state, &value);
                req_str = MC_request_to_string(prev_req, value);
                XBT_DEBUG("%s (state=%d)", req_str, state->num);
                xbt_free(req_str);
              }

              if (!MC_state_process_is_done(prev_state, req->issuer))
                MC_state_interleave_process(prev_state, req->issuer);
              else
                XBT_DEBUG("Process %p is in done set", req->issuer);

              break;

            } else if (req->issuer ==
                       MC_state_get_internal_request(prev_state)->issuer) {

              XBT_DEBUG("Simcall %d and %d with same issuer", req->call,
                        MC_state_get_internal_request(prev_state)->call);
              break;

            } else {

              XBT_DEBUG
                  ("Simcall %d, process %lu (state %d) and simcall %d, process %lu (state %d) are independant",
                   req->call, req->issuer->pid, state->num,
                   MC_state_get_internal_request(prev_state)->call,
                   MC_state_get_internal_request(prev_state)->issuer->pid,
                   prev_state->num);

            }
          }
        }

        if (MC_state_interleave_size(state)
            && xbt_fifo_size(mc_stack) < _sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num,
                    xbt_fifo_size(mc_stack) + 1);
          if (_sg_mc_checkpoint) {
            if (state->system_state != NULL) {
              MC_restore_snapshot(state->system_state);
              xbt_fifo_unshift(mc_stack, state);
              MC_SET_STD_HEAP;
            } else {
              pos = xbt_fifo_size(mc_stack);
              item = xbt_fifo_get_first_item(mc_stack);
              while (pos > 0) {
                restored_state = (mc_state_t) xbt_fifo_get_item_content(item);
                if (restored_state->system_state != NULL) {
                  break;
                } else {
                  item = xbt_fifo_get_next_item(item);
                  pos--;
                }
              }
              MC_restore_snapshot(restored_state->system_state);
              xbt_fifo_unshift(mc_stack, state);
              MC_SET_STD_HEAP;
              MC_replay(mc_stack, pos);
            }
          } else {
            xbt_fifo_unshift(mc_stack, state);
            MC_SET_STD_HEAP;
            MC_replay(mc_stack, -1);
          }
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
