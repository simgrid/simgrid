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
  xbt_fifo_unshift(mc_stack_safety_stateless, initial_state);
  MC_UNSET_RAW_MEM;

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_SET_RAW_MEM;
  /* Get an enabled process and insert it in the interleave set of the initial state */
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
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
  int value;
  smx_req_t req = NULL, prev_req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;

  while (xbt_fifo_size(mc_stack_safety_stateless) > 0) {

    /* Get current state */
    state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_safety_stateless));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (state=%p)(%u interleave)",
           xbt_fifo_size(mc_stack_safety_stateless), state,
           MC_state_interleave_size(state));

    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack_safety_stateless) < MAX_DEPTH &&
        (req = MC_state_get_request(state, &value))) {

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
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
      xbt_fifo_unshift(mc_stack_safety_stateless, next_state);

      /* Get an enabled process and insert it in the interleave set of the next state */
      xbt_swag_foreach(process, simix_global->process_list){
        if(MC_process_is_enabled(process)){
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
      XBT_DEBUG("There are no more processes to interleave.");

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_safety_stateless);
      MC_state_delete(state);
      MC_UNSET_RAW_MEM;

      /* Check for deadlocks */
      if(MC_deadlock_check()){
        MC_show_deadlock(NULL);
        return;
      }

      MC_SET_RAW_MEM;
      /* Traverse the stack backwards until a state with a non empty interleave
         set is found, deleting all the states that have it empty in the way.
         For each deleted state, check if the request that has generated it 
         (from it's predecesor state), depends on any other previous request 
         executed before it. If it does then add it to the interleave set of the
         state that executed that previous request. */
      while ((state = xbt_fifo_shift(mc_stack_safety_stateless)) != NULL) {
        req = MC_state_get_internal_request(state);
        xbt_fifo_foreach(mc_stack_safety_stateless, item, prev_state, mc_state_t) {
          if(MC_request_depend(req, MC_state_get_internal_request(prev_state))){
            if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
              XBT_DEBUG("Dependent Transitions:");
              prev_req = MC_state_get_executed_request(prev_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, prev_state);
              xbt_free(req_str);
              prev_req = MC_state_get_executed_request(state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, state);
              xbt_free(req_str);              
            }

            if(!MC_state_process_is_done(prev_state, req->issuer))
              MC_state_interleave_process(prev_state, req->issuer);
            else
              XBT_DEBUG("Process %p is in done set", req->issuer);

            break;
          }
        }
        if (MC_state_interleave_size(state)) {
          /* We found a back-tracking point, let's loop */
          xbt_fifo_unshift(mc_stack_safety_stateless, state);
          XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_stack_safety_stateless));
          MC_UNSET_RAW_MEM;
          MC_replay(mc_stack_safety_stateless);
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


/********************* DPOR stateful *********************/

mc_state_ws_t new_state_ws(mc_snapshot_t s, mc_state_t gs){
  mc_state_ws_t sws = NULL;
  sws = xbt_new0(s_mc_state_ws_t, 1);
  sws->system_state = s;
  sws->graph_state = gs;
  return sws;
}

void MC_dpor_stateful_init(){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("DPOR (stateful) init");
  XBT_DEBUG("**************************************************");

  mc_state_t initial_graph_state;
  smx_process_t process; 
  mc_snapshot_t initial_system_snapshot;
  mc_state_ws_t initial_state ;
  
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_system_snapshot = xbt_new0(s_mc_snapshot_t, 1);

  initial_graph_state = MC_state_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
      break;
    }
  }

  MC_take_snapshot(initial_system_snapshot);

  initial_state = new_state_ws(initial_system_snapshot, initial_graph_state);
  xbt_fifo_unshift(mc_stack_safety_stateful, initial_state);

  MC_UNSET_RAW_MEM;

}

void MC_dpor_stateful(){

  smx_process_t process = NULL;
  
  if(xbt_fifo_size(mc_stack_safety_stateful) == 0)
    return;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL, prev_req = NULL;
  char *req_str;
  xbt_fifo_item_t item = NULL;

  mc_snapshot_t next_snapshot;
  mc_state_ws_t current_state;
  mc_state_ws_t prev_state;
  mc_state_ws_t next_state;
 
  while(xbt_fifo_size(mc_stack_safety_stateful) > 0){

    current_state = (mc_state_ws_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_safety_stateful));

    
    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Depth : %d, State : %p , %u interleave", xbt_fifo_size(mc_stack_safety_stateful),current_state, MC_state_interleave_size(current_state->graph_state));

    mc_stats->visited_states++;
    if(mc_stats->expanded_states>1100)
      exit(0);
    //sleep(1);

    if((xbt_fifo_size(mc_stack_safety_stateful) < MAX_DEPTH) && (req = MC_state_get_request(current_state->graph_state, &value))){

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
	req_str = MC_request_to_string(req, value);
	//XBT_INFO("Visited states = %lu",mc_stats->visited_states );
	XBT_DEBUG("Execute: %s",req_str);
	xbt_free(req_str);
      }

      MC_state_set_executed_request(current_state->graph_state, req, value);
      mc_stats->executed_transitions++;
      
      /* Answer the request */
      SIMIX_request_pre(req, value);
      
      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();
      
      /* Create the new expanded graph_state */
      MC_SET_RAW_MEM;
      
      next_graph_state = MC_state_new();
      
      /* Get an enabled process and insert it in the interleave set of the next graph_state */
      xbt_swag_foreach(process, simix_global->process_list){
	if(MC_process_is_enabled(process)){
	  MC_state_interleave_process(next_graph_state, process);
	  break;
	}
      }

      next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
      MC_take_snapshot(next_snapshot);

      next_state = new_state_ws(next_snapshot, next_graph_state);
      xbt_fifo_unshift(mc_stack_safety_stateful, next_state);
      
      MC_UNSET_RAW_MEM;

    }else{
      XBT_DEBUG("There are no more processes to interleave.");
      
      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_safety_stateful);
      MC_UNSET_RAW_MEM;

      /* Check for deadlocks */
      if(MC_deadlock_check()){
        MC_show_deadlock_stateful(NULL);
        return;
      }

      MC_SET_RAW_MEM;
      while((current_state = xbt_fifo_shift(mc_stack_safety_stateful)) != NULL){
	req = MC_state_get_internal_request(current_state->graph_state);
	xbt_fifo_foreach(mc_stack_safety_stateful, item, prev_state, mc_state_ws_t) {
          if(MC_request_depend(req, MC_state_get_internal_request(prev_state->graph_state))){
            if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
              XBT_DEBUG("Dependent Transitions:");
              prev_req = MC_state_get_executed_request(prev_state->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, prev_state->graph_state);
              xbt_free(req_str);
              prev_req = MC_state_get_executed_request(current_state->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, current_state->graph_state);
              xbt_free(req_str);              
            }

            if(!MC_state_process_is_done(prev_state->graph_state, req->issuer)){
              MC_state_interleave_process(prev_state->graph_state, req->issuer);
	      
	    } else {
              XBT_DEBUG("Process %p is in done set", req->issuer);
	    }

            break;
          }
        }

	if(MC_state_interleave_size(current_state->graph_state)){
	  MC_restore_snapshot(current_state->system_state);
	  xbt_fifo_unshift(mc_stack_safety_stateful, current_state);
	  XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_stack_safety_stateful));
	  MC_UNSET_RAW_MEM;
	  break;
	}
      }

      MC_UNSET_RAW_MEM;

    } 
  }
  MC_UNSET_RAW_MEM;
  return;
}



