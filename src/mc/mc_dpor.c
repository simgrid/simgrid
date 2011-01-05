/* Copyright (c) 2008 Martin Quinson, Cristian Rosa.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dpor, mc,
                                "Logging specific to MC DPOR exploration");

/**
 *  \brief Initialize the DPOR exploration algorithm
 */
void MC_dpor_init()
{
  mc_state_t initial_state = NULL;
  smx_process_t process;
  
  /* Create the initial state and push it into the exploration stack */
  MC_SET_RAW_MEM;
  initial_state = MC_state_new();
  xbt_fifo_unshift(mc_stack, initial_state);
  MC_UNSET_RAW_MEM;

  DEBUG0("**************************************************");
  DEBUG0("Initial state");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_SET_RAW_MEM;
  /* Get an enabled process and insert it in the interleave set of the initial state */
  xbt_swag_foreach(process, simix_global->process_list){
    if(SIMIX_process_is_enabled(process)){
      MC_state_interleave_process(initial_state, process);
      break;
    }
  }
  MC_UNSET_RAW_MEM;
    
  /* FIXME: Update Statistics 
  mc_stats->state_size +=
      xbt_setset_set_size(initial_state->enabled_transitions); */
}


/**
 * 	\brief Perform the model-checking operation using a depth-first search exploration
 *         with Dynamic Partial Order Reductions
 */
void MC_dpor(void)
{
  char *req_str;
  unsigned int value;
  smx_req_t req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;

  while (xbt_fifo_size(mc_stack) > 0) {

    /* Get current state */
    state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));

    DEBUG0("**************************************************");
    DEBUG3("Exploration detph=%d (state=%p)(%u interleave)",
           xbt_fifo_size(mc_stack), state,
           MC_state_interleave_size(state));

    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack) < MAX_DEPTH &&
        (req = MC_state_get_request(state, &value))) {

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req);
        DEBUG2("Execute: %s (%u)", req_str, value);
        xbt_free(req_str);
      }

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      /* Answer the request */
      SIMIX_request_pre(req, value); /* After this call req is no longer usefull */

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_RAW_MEM;
      next_state = MC_state_new();
      xbt_fifo_unshift(mc_stack, next_state);


      /* Get an enabled process and insert it in the interleave set of the next state */
      xbt_swag_foreach(process, simix_global->process_list){
        if(SIMIX_process_is_enabled(process)){
          MC_state_interleave_process(next_state, process);
          break;
        }
      }
      MC_UNSET_RAW_MEM;

      /* FIXME: Update Statistics
      mc_stats->state_size +=
          xbt_setset_set_size(next_state->enabled_transitions);*/

      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {
      DEBUG0("There are no more processes to interleave.");

      /* Check for deadlocks */
      if(MC_deadlock_check()){
        MC_show_deadlock(&process->request);
        return;
      }

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack);
      MC_state_delete(state);

      /* Traverse the stack backwards until a state with a non empty interleave
         set is found, deleting all the states that have it empty in the way.
         For each deleted state, check if the request that has generated it 
         (from it's predecesor state), depends on any other previous request 
         executed before it. If it does then add it to the interleave set of the
         state that executed that previous request. */
      while ((state = xbt_fifo_shift(mc_stack)) != NULL) {
        req = MC_state_get_executed_request(state, &value);
        xbt_fifo_foreach(mc_stack, item, prev_state, mc_state_t) {
          if(MC_request_depend(req, MC_state_get_executed_request(prev_state, &value))){
            if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
              DEBUG0("Dependent Transitions:");
              req_str = MC_request_to_string(MC_state_get_executed_request(prev_state, &value));
              DEBUG2("%s (state=%p)", req_str, prev_state);
              xbt_free(req_str);
              req_str = MC_request_to_string(req);
              DEBUG2("%s (state=%p)", req_str, state);
              xbt_free(req_str);              
            }

            if(!MC_state_process_is_done(prev_state, req->issuer))
              MC_state_interleave_process(prev_state, req->issuer);
            else
              DEBUG1("Process %p is in done set", req->issuer);

            break;
          }
        }
        if (MC_state_interleave_size(state)) {
          /* We found a back-tracking point, let's loop */
          xbt_fifo_unshift(mc_stack, state);
          DEBUG1("Back-tracking to depth %d", xbt_fifo_size(mc_stack));
          MC_UNSET_RAW_MEM;
          MC_replay(mc_stack);
          break;
        } else {
          MC_state_delete(state);
        }
      }
      MC_UNSET_RAW_MEM;
    }
  }
  MC_UNSET_RAW_MEM;
  return;
}
