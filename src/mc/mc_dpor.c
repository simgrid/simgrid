/* Copyright (c) 2008-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dpor, mc,
                                "Logging specific to MC DPOR exploration");

/********** Global variables **********/

xbt_dynar_t visited_states;
xbt_dict_t first_enabled_state;

/********** Static functions ***********/

static void visited_state_free(mc_visited_state_t state){
  if(state){
    MC_free_snapshot(state->system_state);
    xbt_free(state);
  }
}

static void visited_state_free_voidp(void *s){
  visited_state_free((mc_visited_state_t) * (void **) s);
}

static mc_visited_state_t visited_state_new(){

  mc_visited_state_t new_state = NULL;
  new_state = xbt_new0(s_mc_visited_state_t, 1);
  new_state->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  new_state->nb_processes = xbt_swag_size(simix_global->process_list);
  new_state->system_state = MC_take_snapshot();
  new_state->num = mc_stats->expanded_states;
  new_state->other_num = -1;

  return new_state;
  
}

static int get_search_interval(xbt_dynar_t all_states, mc_visited_state_t state, int *min, int *max){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  int cursor = 0, previous_cursor, next_cursor;
  mc_visited_state_t state_test;
  int start = 0;
  int end = xbt_dynar_length(all_states) - 1;
  
  while(start <= end){
    cursor = (start + end) / 2;
    state_test = (mc_visited_state_t)xbt_dynar_get_as(all_states, cursor, mc_visited_state_t);
    if(state_test->nb_processes < state->nb_processes){
      start = cursor + 1;
    }else if(state_test->nb_processes > state->nb_processes){
      end = cursor - 1;
    }else{
      if(state_test->heap_bytes_used < state->heap_bytes_used){
        start = cursor +1;
      }else if(state_test->heap_bytes_used > state->heap_bytes_used){
        end = cursor - 1;
      }else{
        *min = *max = cursor;
        previous_cursor = cursor - 1;
        while(previous_cursor >= 0){
          state_test = (mc_visited_state_t)xbt_dynar_get_as(all_states, previous_cursor, mc_visited_state_t);
          if(state_test->nb_processes != state->nb_processes || state_test->heap_bytes_used != state->heap_bytes_used)
            break;
          *min = previous_cursor;
          previous_cursor--;
        }
        next_cursor = cursor + 1;
        while(next_cursor < xbt_dynar_length(all_states)){
          state_test = (mc_visited_state_t)xbt_dynar_get_as(all_states, next_cursor, mc_visited_state_t);
          if(state_test->nb_processes != state->nb_processes || state_test->heap_bytes_used != state->heap_bytes_used)
            break;
          *max = next_cursor;
          next_cursor++;
        }
        if(!raw_mem_set)
          MC_UNSET_RAW_MEM;
        return -1;
      }
     }
  }

  if(!raw_mem_set)
    MC_UNSET_RAW_MEM;

  return cursor;
}

static int is_visited_state(){

  if(_sg_mc_visited == 0)
    return -1;

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  mc_visited_state_t new_state = visited_state_new();
  
  if(xbt_dynar_is_empty(visited_states)){

    xbt_dynar_push(visited_states, &new_state); 

    if(!raw_mem_set)
      MC_UNSET_RAW_MEM;

    return -1;

  }else{

    int min = -1, max = -1, index;
    //int res;
    mc_visited_state_t state_test;
    int cursor;

    index = get_search_interval(visited_states, new_state, &min, &max);

    if(min != -1 && max != -1){
      /*res = xbt_parmap_mc_apply(parmap, snapshot_compare, xbt_dynar_get_ptr(visited_states, min), (max-min)+1, new_state);
      if(res != -1){
        state_test = (mc_visited_state_t)xbt_dynar_get_as(visited_states, (min+res)-1, mc_visited_state_t);
        if(state_test->other_num == -1)
          new_state->other_num = state_test->num;
        else
          new_state->other_num = state_test->other_num;
        if(dot_output == NULL)
          XBT_DEBUG("State %d already visited ! (equal to state %d)", new_state->num, state_test->num);
        else
          XBT_DEBUG("State %d already visited ! (equal to state %d (state %d in dot_output))", new_state->num, state_test->num, new_state->other_num);
        xbt_dynar_remove_at(visited_states, (min + res) - 1, NULL);
        xbt_dynar_insert_at(visited_states, (min+res) - 1, &new_state);
        if(!raw_mem_set)
          MC_UNSET_RAW_MEM;
        return new_state->other_num;
        }*/
      cursor = min;
      while(cursor <= max){
        state_test = (mc_visited_state_t)xbt_dynar_get_as(visited_states, cursor, mc_visited_state_t);
        if(snapshot_compare(state_test, new_state) == 0){
          if(state_test->other_num == -1)
            new_state->other_num = state_test->num;
          else
            new_state->other_num = state_test->other_num;
          if(dot_output == NULL)
            XBT_DEBUG("State %d already visited ! (equal to state %d)", new_state->num, state_test->num);
          else
            XBT_DEBUG("State %d already visited ! (equal to state %d (state %d in dot_output))", new_state->num, state_test->num, new_state->other_num);
          xbt_dynar_remove_at(visited_states, cursor, NULL);
          xbt_dynar_insert_at(visited_states, cursor, &new_state);
          if(!raw_mem_set)
            MC_UNSET_RAW_MEM;
          return new_state->other_num;
        }
        cursor++;
      }
      xbt_dynar_insert_at(visited_states, min, &new_state);
    }else{
      state_test = (mc_visited_state_t)xbt_dynar_get_as(visited_states, index, mc_visited_state_t);
      if(state_test->nb_processes < new_state->nb_processes){
        xbt_dynar_insert_at(visited_states, index+1, &new_state);
      }else{
        if(state_test->heap_bytes_used < new_state->heap_bytes_used)
          xbt_dynar_insert_at(visited_states, index + 1, &new_state);
        else
          xbt_dynar_insert_at(visited_states, index, &new_state);
      }
    }

    if(xbt_dynar_length(visited_states) > _sg_mc_visited){
      int min2 = mc_stats->expanded_states;
      unsigned int cursor2 = 0;
      unsigned int index2 = 0;
      xbt_dynar_foreach(visited_states, cursor2, state_test){
        if(state_test->num < min2){
          index2 = cursor2;
          min2 = state_test->num;
        }
      }
      xbt_dynar_remove_at(visited_states, index2, NULL);
    }

    if(!raw_mem_set)
      MC_UNSET_RAW_MEM;
    
    return -1;
    
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
  if(_sg_mc_visited > 0)
    visited_states = xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);

  first_enabled_state = xbt_dict_new_homogeneous(&xbt_free_f);

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
      if(mc_reduce_kind != e_mc_reduce_none)
        break;
    }
  }

  xbt_fifo_unshift(mc_stack_safety, initial_state);

  /* To ensure the soundness of DPOR, we have to keep a list of 
     processes which are still enabled at each step of the exploration. 
     If max depth is reached, we interleave them in the state in which they have 
     been enabled for the first time. */
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      char *key = bprintf("%lu", process->pid);
      char *data = bprintf("%d", xbt_fifo_size(mc_stack_safety));
      xbt_dict_set(first_enabled_state, key, data, NULL);
      xbt_free(key);
    }
  }

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

  char *req_str = NULL;
  int value;
  smx_simcall_t req = NULL, prev_req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL, restore_state=NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  int pos;
  int visited_state = -1;
  int enabled = 0;

  while (xbt_fifo_size(mc_stack_safety) > 0) {

    /* Get current state */
    state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_safety));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (state=%p, num %d)(%u interleave, user_max_depth %d)",
              xbt_fifo_size(mc_stack_safety), state, state->num,
              MC_state_interleave_size(state), user_max_depth_reached);
      
    /* Update statistics */
    mc_stats->visited_states++;

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack_safety) <= _sg_mc_max_depth && !user_max_depth_reached &&
        (req = MC_state_get_request(state, &value)) && visited_state == -1) {

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
        xbt_free(req_str);
      }
      
      MC_SET_RAW_MEM;
      if(dot_output != NULL)
        req_str = MC_request_get_dot_output(req, value);
      MC_UNSET_RAW_MEM;

      MC_state_set_executed_request(state, req, value);
      mc_stats->executed_transitions++;

      MC_SET_RAW_MEM;
      char *key = bprintf("%lu", req->issuer->pid);
      xbt_dict_remove(first_enabled_state, key); 
      xbt_free(key);
      MC_UNSET_RAW_MEM;

      /* Answer the request */
      SIMIX_simcall_pre(req, value); /* After this call req is no longer usefull */

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_RAW_MEM;

      next_state = MC_state_new();

      if((visited_state = is_visited_state()) == -1){
     
        /* Get an enabled process and insert it in the interleave set of the next state */
        xbt_swag_foreach(process, simix_global->process_list){
          if(MC_process_is_enabled(process)){
            MC_state_interleave_process(next_state, process);
            if(mc_reduce_kind != e_mc_reduce_none)
              break;
          }
        }

        if(_sg_mc_checkpoint && ((xbt_fifo_size(mc_stack_safety) + 1) % _sg_mc_checkpoint == 0)){
          next_state->system_state = MC_take_snapshot();
        }

        if(dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, next_state->num, req_str);

      }else{

        if(dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, visited_state, req_str);

      }

      xbt_fifo_unshift(mc_stack_safety, next_state);

      /* Insert in dict all enabled processes, if not included yet */
      xbt_swag_foreach(process, simix_global->process_list){
        if(MC_process_is_enabled(process)){
          char *key = bprintf("%lu", process->pid);
          if(xbt_dict_get_or_null(first_enabled_state, key) == NULL){
            char *data = bprintf("%d", xbt_fifo_size(mc_stack_safety));
            xbt_dict_set(first_enabled_state, key, data, NULL); 
          }
          xbt_free(key);
        }
      }
      
      if(dot_output != NULL)
        xbt_free(req_str);

      MC_UNSET_RAW_MEM;

      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {

      if((xbt_fifo_size(mc_stack_safety) > _sg_mc_max_depth) || user_max_depth_reached || visited_state != -1){  

        if(user_max_depth_reached && visited_state == -1)
          XBT_DEBUG("User max depth reached !");
        else if(visited_state == -1)
          XBT_WARN("/!\\ Max depth reached ! /!\\ ");

        visited_state = -1;

        /* Interleave enabled processes in the state in which they have been enabled for the first time */
        xbt_swag_foreach(process, simix_global->process_list){
          if(MC_process_is_enabled(process)){
            char *key = bprintf("%lu", process->pid);
            enabled = (int)strtoul(xbt_dict_get_or_null(first_enabled_state, key), 0, 10);
            xbt_free(key);
            mc_state_t state_test = NULL;
            xbt_fifo_item_t item = NULL;
            int cursor = xbt_fifo_size(mc_stack_safety);
            xbt_fifo_foreach(mc_stack_safety, item, state_test, mc_state_t){
              if(cursor-- == enabled){ 
                if(!MC_state_process_is_done(state_test, process) && state_test->num != state->num){ 
                  XBT_DEBUG("Interleave process %lu in state %d", process->pid, state_test->num);
                  MC_state_interleave_process(state_test, process);
                  break;
                }
              }
            } 
          }
        }

      }else{

        XBT_DEBUG("There are no more processes to interleave. (depth %d)", xbt_fifo_size(mc_stack_safety) + 1);

      }

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_safety);
      MC_state_delete(state);
      XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack_safety) + 1);
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
        if(mc_reduce_kind != e_mc_reduce_none){
          req = MC_state_get_internal_request(state);
          xbt_fifo_foreach(mc_stack_safety, item, prev_state, mc_state_t) {
            if(MC_request_depend(req, MC_state_get_internal_request(prev_state))){
              if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
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

              if(!MC_state_process_is_done(prev_state, req->issuer))
                MC_state_interleave_process(prev_state, req->issuer);
              else
                XBT_DEBUG("Process %p is in done set", req->issuer);

              break;

            }else if(req->issuer == MC_state_get_internal_request(prev_state)->issuer){

              XBT_DEBUG("Simcall %d and %d with same issuer", req->call, MC_state_get_internal_request(prev_state)->call);
              break;

            }else{

              XBT_DEBUG("Simcall %d, process %lu (state %d) and simcall %d, process %lu (state %d) are independant", req->call, req->issuer->pid, state->num, MC_state_get_internal_request(prev_state)->call, MC_state_get_internal_request(prev_state)->issuer->pid, prev_state->num);

            }
          }
        }
             
        if (MC_state_interleave_size(state) && xbt_fifo_size(mc_stack_safety) < _sg_mc_max_depth) {
          /* We found a back-tracking point, let's loop */
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num, xbt_fifo_size(mc_stack_safety) + 1);
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
          XBT_DEBUG("Back-tracking to state %d at depth %d", state->num, xbt_fifo_size(mc_stack_safety));
          break;
        } else {
          XBT_DEBUG("Delete state %d at depth %d", state->num, xbt_fifo_size(mc_stack_safety) + 1); 
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




