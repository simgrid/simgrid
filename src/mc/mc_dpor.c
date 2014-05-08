/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dpor, mc,
                                "Logging specific to MC DPOR exploration");

/********** Global variables **********/

xbt_dynar_t visited_states;
xbt_dict_t first_enabled_state;
xbt_dynar_t initial_communications_pattern;
xbt_dynar_t incomplete_communications_pattern;
xbt_dynar_t communications_pattern;
int nb_comm_pattern;

/********** Static functions ***********/

static void comm_pattern_free(mc_comm_pattern_t p){
  xbt_free(p->rdv);
  xbt_free(p->data);
  xbt_free(p);
  p = NULL;
}

static void comm_pattern_free_voidp( void *p){
  comm_pattern_free((mc_comm_pattern_t) * (void **)p);
}

static mc_comm_pattern_t get_comm_pattern_from_idx(xbt_dynar_t pattern, unsigned int *idx, e_smx_comm_type_t type, unsigned long proc){
  mc_comm_pattern_t current_comm;
  while(*idx < xbt_dynar_length(pattern)){
    current_comm = (mc_comm_pattern_t)xbt_dynar_get_as(pattern, *idx, mc_comm_pattern_t);
    if(current_comm->type == type && type == SIMIX_COMM_SEND){
      if(current_comm->src_proc == proc)
        return current_comm;
    }else if(current_comm->type == type && type == SIMIX_COMM_RECEIVE){
      if(current_comm->dst_proc == proc)
        return current_comm;
    }
    (*idx)++;
  }
  return NULL;
}

static int compare_comm_pattern(mc_comm_pattern_t comm1, mc_comm_pattern_t comm2){
  if(strcmp(comm1->rdv, comm2->rdv) != 0)
    return 1;
  if(comm1->src_proc != comm2->src_proc)
    return 1;
  if(comm1->dst_proc != comm2->dst_proc)
    return 1;
  if(comm1->data_size != comm2->data_size)
    return 1;
  if(memcmp(comm1->data, comm2->data, comm1->data_size) != 0)
    return 1;
  return 0;
}

static void deterministic_pattern(xbt_dynar_t initial_pattern, xbt_dynar_t pattern){
  unsigned int cursor = 0, send_index = 0, recv_index = 0;
  mc_comm_pattern_t comm1, comm2;
  int comm_comparison = 0;
  int current_process = 0;
  while(current_process < simix_process_maxpid){
    while(cursor < xbt_dynar_length(initial_pattern)){
      comm1 = (mc_comm_pattern_t)xbt_dynar_get_as(initial_pattern, cursor, mc_comm_pattern_t);
      if(comm1->type == SIMIX_COMM_SEND && comm1->src_proc == current_process){
        comm2 = get_comm_pattern_from_idx(pattern, &send_index, comm1->type, current_process);
        comm_comparison = compare_comm_pattern(comm1, comm2);
        if(comm_comparison == 1){
          initial_state_safety->send_deterministic = 0;
          initial_state_safety->comm_deterministic = 0;
          return;
        }
        send_index++;
      }else if(comm1->type == SIMIX_COMM_RECEIVE && comm1->dst_proc == current_process){
        comm2 = get_comm_pattern_from_idx(pattern, &recv_index, comm1->type, current_process);
        comm_comparison = compare_comm_pattern(comm1, comm2);
        if(comm_comparison == 1){
          initial_state_safety->comm_deterministic = 0;
          if(!_sg_mc_send_determinism)
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

static int complete_comm_pattern(xbt_dynar_t list, mc_comm_pattern_t pattern){
  mc_comm_pattern_t current_pattern;
  unsigned int cursor = 0;
  int index;
  xbt_dynar_foreach(incomplete_communications_pattern, cursor, index){
    current_pattern = (mc_comm_pattern_t)xbt_dynar_get_as(list, index, mc_comm_pattern_t);
    if(current_pattern->comm == pattern->comm){
      current_pattern->src_proc = pattern->comm->comm.src_proc->pid;
      current_pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
      current_pattern->src_host = simcall_host_get_name(pattern->comm->comm.src_proc->smx_host);
      current_pattern->dst_host = simcall_host_get_name(pattern->comm->comm.dst_proc->smx_host);
      if(current_pattern->data_size == -1){
        current_pattern->data_size = pattern->comm->comm.src_buff_size;
        current_pattern->data = xbt_malloc0(current_pattern->data_size);
        memcpy(current_pattern->data, current_pattern->comm->comm.src_buff, current_pattern->data_size);
      }
      current_pattern->matched_comm = pattern->num;
      current_pattern->completed = 1;
      xbt_dynar_remove_at(incomplete_communications_pattern, cursor, NULL);
      return current_pattern->num;
    }
  }
  return -1;
}

void get_comm_pattern(xbt_dynar_t list, smx_simcall_t request, int call){
  mc_comm_pattern_t pattern = NULL;
  pattern = xbt_new0(s_mc_comm_pattern_t, 1);
  pattern->num = ++nb_comm_pattern;
  pattern->completed = 0;
  pattern->data_size = -1;
  if(call == 1){ // ISEND
    pattern->comm = simcall_comm_isend__get__result(request);
    pattern->type = SIMIX_COMM_SEND;
    if(pattern->comm->comm.dst_proc != NULL){ 
      pattern->matched_comm = complete_comm_pattern(list, pattern);
      pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
      pattern->dst_host = simcall_host_get_name(pattern->comm->comm.dst_proc->smx_host);
      pattern->completed = 1;
    }
    pattern->src_proc = pattern->comm->comm.src_proc->pid;
    pattern->src_host = simcall_host_get_name(request->issuer->smx_host);
    pattern->data_size = pattern->comm->comm.src_buff_size;
    pattern->data=xbt_malloc0(pattern->data_size);
    memcpy(pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
  }else{ // IRECV
    pattern->comm = simcall_comm_irecv__get__result(request);
    pattern->type = SIMIX_COMM_RECEIVE;
    if(pattern->comm->comm.src_proc != NULL){
      pattern->matched_comm = complete_comm_pattern(list, pattern);
      pattern->src_proc = pattern->comm->comm.src_proc->pid;
      pattern->src_host = simcall_host_get_name(request->issuer->smx_host);
      pattern->completed = 1;
      pattern->data_size = pattern->comm->comm.src_buff_size;
      pattern->data=xbt_malloc0(pattern->data_size);
      memcpy(pattern->data, pattern->comm->comm.src_buff, pattern->data_size);
    }
    pattern->dst_proc = pattern->comm->comm.dst_proc->pid;
    pattern->dst_host = simcall_host_get_name(pattern->comm->comm.dst_proc->smx_host);
  }
  
  if(pattern->comm->comm.rdv != NULL)
    pattern->rdv = strdup(pattern->comm->comm.rdv->name);
  else
    pattern->rdv = strdup(pattern->comm->comm.rdv_cpy->name);
  
  xbt_dynar_push(list, &pattern);

  if(!pattern->completed)
    xbt_dynar_push_as(incomplete_communications_pattern, int, xbt_dynar_length(list) - 1);

}

static void print_communications_pattern(xbt_dynar_t comms_pattern){
  unsigned int cursor = 0;
  mc_comm_pattern_t current_comm;
  xbt_dynar_foreach(comms_pattern, cursor, current_comm){
    if(current_comm->type == SIMIX_COMM_SEND)
      XBT_INFO("[(%lu) %s -> %s] %s ", current_comm->src_proc, current_comm->src_host, current_comm->dst_host, "iSend");
    else
      XBT_INFO("[(%lu) %s <- %s] %s ", current_comm->dst_proc, current_comm->dst_host, current_comm->src_host, "iRecv");
  }
}

static void visited_state_free(mc_visited_state_t state){
  if(state){
    MC_free_snapshot(state->system_state);
    xbt_free(state);
  }
}

static void visited_state_free_voidp(void *s){
  visited_state_free((mc_visited_state_t) * (void **) s);
}

/** \brief Save the current state
 *
 *  \return Snapshot of the current state.
 */
static mc_visited_state_t visited_state_new(){

  mc_visited_state_t new_state = NULL;
  new_state = xbt_new0(s_mc_visited_state_t, 1);
  new_state->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  new_state->nb_processes = xbt_swag_size(simix_global->process_list);
  new_state->system_state = MC_take_snapshot(mc_stats->expanded_states);
  new_state->num = mc_stats->expanded_states;
  new_state->other_num = -1;

  return new_state;
  
}

/** \brief Find a suitable subrange of candidate duplicates for a given state
 *
 *  \param all_ pairs dynamic array of states with candidate duplicates of the current state;
 *  \param pair current state;
 *  \param min (output) index of the beginning of the the subrange
 *  \param max (output) index of the enf of the subrange
 *
 *  Given a suitably ordered array of state, this function extracts a subrange
 *  (with index *min <= i <= *max) with candidate duplicates of the given state.
 *  This function uses only fast discriminating criterions and does not use the
 *  full state comparison algorithms.
 *
 *  The states in all_pairs MUST be ordered using a (given) weak order
 *  (based on nb_processes and heap_bytes_used).
 *  The subrange is the subrange of "equivalence" of the given state.
 */
static int get_search_interval(xbt_dynar_t all_states, mc_visited_state_t state, int *min, int *max){
  XBT_VERB("Searching interval for state %i: nd_processes=%zu heap_bytes_used=%zu",
    state->num, (size_t)state->nb_processes, (size_t)state->heap_bytes_used);

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

/** \brief Take a snapshot the current state and process it.
 *
 *  \return number of the duplicate state or -1 (not visited)
 */
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

      // Parallell implementation
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
          // The state has been visited:

          if(state_test->other_num == -1)
            new_state->other_num = state_test->num;
          else
            new_state->other_num = state_test->other_num;
          if(dot_output == NULL)
            XBT_DEBUG("State %d already visited ! (equal to state %d)", new_state->num, state_test->num);
          else
            XBT_DEBUG("State %d already visited ! (equal to state %d (state %d in dot_output))", new_state->num, state_test->num, new_state->other_num);

          // Replace the old state with the new one (why?):
          xbt_dynar_remove_at(visited_states, cursor, NULL);
          xbt_dynar_insert_at(visited_states, cursor, &new_state);

          if(!raw_mem_set)
            MC_UNSET_RAW_MEM;
          return new_state->other_num;
        }
        cursor++;
      }

      // The state has not been visited, add it to the list:
      xbt_dynar_insert_at(visited_states, min, &new_state);

    }else{

      // The state has not been visited: insert the state in the dynamic array.
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

    // We have reached the maximum number of stored states;
    if(xbt_dynar_length(visited_states) > _sg_mc_visited){

      // Find the (index of the) older state:
      int min2 = mc_stats->expanded_states;
      unsigned int cursor2 = 0;
      unsigned int index2 = 0;
      xbt_dynar_foreach(visited_states, cursor2, state_test){
        if(state_test->num < min2){
          index2 = cursor2;
          min2 = state_test->num;
        }
      }

      // and drop it:
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

  if(_sg_mc_visited > 0)
    visited_states = xbt_dynar_new(sizeof(mc_visited_state_t), visited_state_free_voidp);

  if(mc_reduce_kind == e_mc_reduce_dpor)
    first_enabled_state = xbt_dict_new_homogeneous(&xbt_free_f);

  if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
    initial_communications_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
    communications_pattern = xbt_dynar_new(sizeof(mc_comm_pattern_t), comm_pattern_free_voidp);
    incomplete_communications_pattern = xbt_dynar_new(sizeof(int), NULL);
    nb_comm_pattern = 0;
  }

  initial_state = MC_state_new();

  MC_UNSET_RAW_MEM;

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial state");

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  MC_ignore_heap(simix_global->process_to_run->data, 0);
  MC_ignore_heap(simix_global->process_that_ran->data, 0);

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

  if(mc_reduce_kind == e_mc_reduce_dpor){
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
  }

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


/** \brief Model-check the application using a DFS exploration
 *         with DPOR (Dynamic Partial Order Reductions)
 */
void MC_dpor(void)
{

  char *req_str = NULL;
  int value;
  smx_simcall_t req = NULL, prev_req = NULL;
  mc_state_t state = NULL, prev_state = NULL, next_state = NULL, restored_state=NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  mc_state_t state_test = NULL;
  int pos;
  int visited_state = -1;
  int enabled = 0;
  int interleave_size = 0;
  int comm_pattern = 0;

  while (xbt_fifo_size(mc_stack_safety) > 0) {

    /* Get current state */
    state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_safety));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (state=%p, num %d)(%u interleave, user_max_depth %d, first_enabled_state size : %d)",
              xbt_fifo_size(mc_stack_safety), state, state->num,
              MC_state_interleave_size(state), user_max_depth_reached, xbt_dict_size(first_enabled_state));
      
    interleave_size = MC_state_interleave_size(state);

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

      if(mc_reduce_kind ==  e_mc_reduce_dpor){
        MC_SET_RAW_MEM;
        char *key = bprintf("%lu", req->issuer->pid);
        xbt_dict_remove(first_enabled_state, key); 
        xbt_free(key);
        MC_UNSET_RAW_MEM;
      }

      if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
        if(req->call == SIMCALL_COMM_ISEND)
          comm_pattern = 1;
        else if(req->call == SIMCALL_COMM_IRECV)
          comm_pattern = 2;
      }

      /* Answer the request */
      SIMIX_simcall_pre(req, value); /* After this call req is no longer usefull */

      if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
        MC_SET_RAW_MEM;
        if(comm_pattern != 0){
          if(!initial_state_safety->initial_communications_pattern_done)
            get_comm_pattern(initial_communications_pattern, req, comm_pattern);
          else
            get_comm_pattern(communications_pattern, req, comm_pattern);
        }
        MC_UNSET_RAW_MEM; 
        comm_pattern = 0;
      }

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
          next_state->system_state = MC_take_snapshot(next_state->num);
        }

        if(dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, next_state->num, req_str);

      }else{

        if(dot_output != NULL)
          fprintf(dot_output, "\"%d\" -> \"%d\" [%s];\n", state->num, visited_state, req_str);

      }

      xbt_fifo_unshift(mc_stack_safety, next_state);

      if(mc_reduce_kind ==  e_mc_reduce_dpor){
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

        if(mc_reduce_kind ==  e_mc_reduce_dpor){
          /* Interleave enabled processes in the state in which they have been enabled for the first time */
          xbt_swag_foreach(process, simix_global->process_list){
            if(MC_process_is_enabled(process)){
              char *key = bprintf("%lu", process->pid);
              enabled = (int)strtoul(xbt_dict_get_or_null(first_enabled_state, key), 0, 10);
              xbt_free(key);
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
        }

      }else{

        XBT_DEBUG("There are no more processes to interleave. (depth %d)", xbt_fifo_size(mc_stack_safety) + 1);

      }

      MC_SET_RAW_MEM;

      if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
        if(initial_state_safety->initial_communications_pattern_done){
          if(interleave_size == 0){ /* if (interleave_size > 0), process interleaved but not enabled => "incorrect" path, determinism not evaluated */
            //print_communications_pattern(communications_pattern);
            deterministic_pattern(initial_communications_pattern, communications_pattern);
            if(initial_state_safety->comm_deterministic == 0 && _sg_mc_comms_determinism){
              XBT_INFO("****************************************************");
              XBT_INFO("***** Non-deterministic communications pattern *****");
              XBT_INFO("****************************************************");
              XBT_INFO("Initial communications pattern:");
              print_communications_pattern(initial_communications_pattern);
              XBT_INFO("Communications pattern counter-example:");
              print_communications_pattern(communications_pattern);
              MC_print_statistics(mc_stats);
              return;
            }else if(initial_state_safety->send_deterministic == 0 && _sg_mc_send_determinism){
              XBT_INFO("****************************************************");
              XBT_INFO("***** Non-send-deterministic communications pattern *****");
              XBT_INFO("****************************************************");
              XBT_INFO("Initial communications pattern:");
              print_communications_pattern(initial_communications_pattern);
              XBT_INFO("Communications pattern counter-example:");
              print_communications_pattern(communications_pattern);
              MC_print_statistics(mc_stats);
              return;
            }
          }
        }else{
          initial_state_safety->initial_communications_pattern_done = 1;
        }
      }

      /* Trash the current state, no longer needed */
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
                restored_state = (mc_state_t) xbt_fifo_get_item_content(item);
                if(restored_state->system_state != NULL){
                  break;
                }else{
                  item = xbt_fifo_get_next_item(item);
                  pos--;
                }
              }
              MC_restore_snapshot(restored_state->system_state);
              xbt_fifo_unshift(mc_stack_safety, state);
              MC_UNSET_RAW_MEM;
              MC_replay(mc_stack_safety, pos);
            }
          }else{
            xbt_fifo_unshift(mc_stack_safety, state);
            MC_UNSET_RAW_MEM;
            MC_replay(mc_stack_safety, -1);
          }
          XBT_DEBUG("Back-tracking to state %d at depth %d done", state->num, xbt_fifo_size(mc_stack_safety));
          break;
        } else {
          /*req = MC_state_get_internal_request(state);
          if(_sg_mc_comms_determinism || _sg_mc_send_determinism){
            if(req->call == SIMCALL_COMM_ISEND || req->call == SIMCALL_COMM_IRECV){
              if(!xbt_dynar_is_empty(communications_pattern))
                xbt_dynar_remove_at(communications_pattern, xbt_dynar_length(communications_pattern) - 1, NULL);
            }
            }*/
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




