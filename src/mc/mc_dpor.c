/* Copyright (c) 2008-2012. Da SimGrid Team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dpor, mc,
                                "Logging specific to MC DPOR exploration");

xbt_dynar_t visited_states;

static int is_visited_state(void);

static int is_visited_state(){

  if(_sg_mc_visited == 0)
    return 0;

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  mc_snapshot_t new_state = NULL;
  new_state = MC_take_snapshot();  

  MC_UNSET_RAW_MEM;
  
  if(xbt_dynar_is_empty(visited_states)){

    MC_SET_RAW_MEM;
    xbt_dynar_push(visited_states, &new_state); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
 
    return 0;

  }else{

    MC_SET_RAW_MEM;
    
    unsigned int cursor = 0;
    mc_snapshot_t state_test = NULL;
     
    xbt_dynar_foreach(visited_states, cursor, state_test){
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug))
        XBT_DEBUG("****** Pair visited #%d ******", cursor + 1);
      if(snapshot_compare(new_state, state_test, NULL, NULL) == 0){
        if(raw_mem_set)
          MC_SET_RAW_MEM;
        else
          MC_UNSET_RAW_MEM;
        
        return 1;
      }   
    }

    if(xbt_dynar_length(visited_states) == _sg_mc_visited){
      mc_snapshot_t state_to_remove = NULL;
      xbt_dynar_shift(visited_states, &state_to_remove);
      MC_free_snapshot(state_to_remove);
    }

    xbt_dynar_push(visited_states, &new_state); 
    
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
    
    return 0;
    
  }
}

/**
 *  \brief Initialize the DPOR exploration algorithm
 */
void MC_dpor_init()
{
  
  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  mc_state_t initial_state = NULL;
  smx_process_t process;
  
  /* Create the initial state and push it into the exploration stack */
  MC_SET_RAW_MEM;

  initial_state = MC_state_new();
  visited_states = xbt_dynar_new(sizeof(mc_snapshot_t), NULL);

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
      XBT_DEBUG("Process %lu enabled with simcall %d", process->pid, process->simcall.call);
    }
  }

  xbt_fifo_unshift(mc_stack_safety, initial_state);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


/**
 *   \brief Perform the model-checking operation using a depth-first search exploration
 *         with Dynamic Partial Order Reductions
 */
void MC_dpor(void)
{

  char *req_str;
  int value;
  smx_simcall_t req = NULL, prev_req = NULL;
  s_smx_simcall_t req2;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL, restore_state=NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  int pos, i;
  int interleave_proc[simix_process_maxpid];

  while (xbt_fifo_size(mc_stack_safety) > 0) {

    /* Get current state */
    state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_safety));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (state=%p)(%u interleave)",
              xbt_fifo_size(mc_stack_safety), state,
              MC_state_interleave_size(state));

    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack_safety) < _sg_mc_max_depth &&
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
      SIMIX_simcall_pre(req, value); /* After this call req is no longer usefull */

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_RAW_MEM;

      next_state = MC_state_new();

      if(!is_visited_state()){
     
        /* Get an enabled process and insert it in the interleave set of the next state */
        xbt_swag_foreach(process, simix_global->process_list){
          if(MC_process_is_enabled(process)){
            MC_state_interleave_process(next_state, process);
            XBT_DEBUG("Process %lu enabled with simcall %d", process->pid, process->simcall.call);
          }
        }

        if(_sg_mc_checkpoint && ((xbt_fifo_size(mc_stack_safety) + 1) % _sg_mc_checkpoint == 0)){
          next_state->system_state = MC_take_snapshot();
        }

      }else{

        XBT_DEBUG("State already visited !");
        
      }

      xbt_fifo_unshift(mc_stack_safety, next_state);
      MC_UNSET_RAW_MEM;

      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {

      if(xbt_fifo_size(mc_stack_safety) == _sg_mc_max_depth)  
        XBT_WARN("/!\\ Max depth reached ! /!\\ ");
      else
        XBT_DEBUG("There are no more processes to interleave.");

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_safety);
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
      
      while ((state = xbt_fifo_shift(mc_stack_safety)) != NULL) {
        req = MC_state_get_internal_request(state);
        xbt_fifo_foreach(mc_stack_safety, item, prev_state, mc_state_t) {
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

            break;

          }else if(req->issuer == MC_state_get_internal_request(prev_state)->issuer){

            XBT_DEBUG("Simcall %d and %d with same issuer", req->call, MC_state_get_internal_request(prev_state)->call);
            break;

          }else{

            MC_state_remove_interleave_process(prev_state, req->issuer);
            XBT_DEBUG("Simcall %d in process %lu independant with simcall %d process %lu", req->call, req->issuer->pid, MC_state_get_internal_request(prev_state)->call, MC_state_get_internal_request(prev_state)->issuer->pid);  

          }
        }
       
        if (MC_state_interleave_size(state)) {
          /* We found a back-tracking point, let's loop */
          if(_sg_mc_checkpoint){
            if(state->system_state != NULL){
              MC_restore_snapshot(state->system_state);
              xbt_fifo_unshift(mc_stack_safety, state);
              MC_UNSET_RAW_MEM;
            }else{
              pos = xbt_fifo_size(mc_stack_safety);
              item = xbt_fifo_get_first_item(mc_stack_safety);
              while(pos>0){
                restore_state = (mc_state_t) xbt_fifo_get_item_content(item);
                if(restore_state->system_state != NULL){
                  break;
                }else{
                  item = xbt_fifo_get_next_item(item);
                  pos--;
                }
              }
              MC_restore_snapshot(restore_state->system_state);
              xbt_fifo_unshift(mc_stack_safety, state);
              MC_UNSET_RAW_MEM;
              MC_replay(mc_stack_safety, pos);
            }
          }else{
            xbt_fifo_unshift(mc_stack_safety, state);
            MC_UNSET_RAW_MEM;
            MC_replay(mc_stack_safety, -1);
          }

          MC_SET_RAW_MEM;
          req2 = *req;
          for(i=0; i<simix_process_maxpid; i++)
            interleave_proc[i] = 0;
          i=0;
          while((i < MC_state_interleave_size(state))){
            i++;
            prev_req = MC_state_get_request(state, &value);
            if(prev_req != NULL){
              MC_state_set_executed_request(state, prev_req, value);
              prev_req = MC_state_get_internal_request(state);
              if(MC_request_depend(&req2, prev_req)){
                XBT_DEBUG("Simcall %d in process %lu dependant with simcall %d in process %lu", req2.call, req2.issuer->pid, prev_req->call, prev_req->issuer->pid);  
                interleave_proc[prev_req->issuer->pid] = 1;
              }else{
                XBT_DEBUG("Simcall %d in process %lu independant with simcall %d in process %lu", req2.call, req2.issuer->pid, prev_req->call, prev_req->issuer->pid); 
                MC_state_remove_interleave_process(state, prev_req->issuer);
              }
            }
          }
          xbt_swag_foreach(process, simix_global->process_list){
            if(interleave_proc[process->pid] == 1)
              MC_state_interleave_process(state, process);
          }
          XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_stack_safety));
          MC_UNSET_RAW_MEM;
          break;
        } else {
          MC_state_delete(state);
        }
      }
      MC_UNSET_RAW_MEM;
    }
  }
  MC_print_statistics(mc_stats);
  MC_UNSET_RAW_MEM;  

  return;
}




