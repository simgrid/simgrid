/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"
#include <unistd.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

xbt_dynar_t reached_pairs;
xbt_dynar_t successors;

int create_dump(int pair)
{
   // Try to enable core dumps
  struct rlimit core_limit;
  core_limit.rlim_cur = RLIM_INFINITY;
  core_limit.rlim_max = RLIM_INFINITY;
  
  if(setrlimit(RLIMIT_CORE, &core_limit) < 0)
    fprintf(stderr, "setrlimit: %s\nWarning: core dumps may be truncated or non-existant\n", strerror(errno));
  
  int status;
  switch(fork()){
  case 0:
    // We are the child process -- run the actual program
    xbt_abort();
    break;
    
  case -1:
    // An error occurred, shouldn't happen
    perror("fork");
    return -1;
    
  default:
    // We are the parent process -- wait for the child process to exit
    if(wait(&status) < 0)
      perror("wait");
    if(WIFSIGNALED(status) && WCOREDUMP(status)){
      char *core_name = malloc(20);
      sprintf(core_name,"core_%d", pair); 
      rename("core", core_name);
      free(core_name);
    }
  }

  return 0;
}

int reached(xbt_state_t st){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  mc_pair_reached_t new_pair = NULL;
  new_pair = xbt_new0(s_mc_pair_reached_t, 1);
  new_pair->nb = xbt_dynar_length(reached_pairs) + 1;
  new_pair->automaton_state = st;
  new_pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  new_pair->system_state = MC_take_snapshot_liveness();  
  
  /* Get values of propositional symbols */
  int res;
  int_f_void_t f;
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(new_pair->prop_ato, int, res);
  }
  
  MC_UNSET_RAW_MEM;
  
  if(xbt_dynar_is_empty(reached_pairs) || !compare){

    MC_SET_RAW_MEM;
    /* New pair reached */
    xbt_dynar_push(reached_pairs, &new_pair); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
 
    return 0;

  }else{

    MC_SET_RAW_MEM;
    
    cursor = 0;
    mc_pair_reached_t pair_test = NULL;
     
    xbt_dynar_foreach(reached_pairs, cursor, pair_test){
      XBT_INFO("Pair reached #%d", pair_test->nb);
      if(automaton_state_compare(pair_test->automaton_state, st) == 0){
        if(propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){
          if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
            
            if(raw_mem_set)
              MC_SET_RAW_MEM;
            else
              MC_UNSET_RAW_MEM;
            
            return 1;
          }       
        }else{
          XBT_INFO("Different values of propositional symbols");
        }
      }else{
        XBT_INFO("Different automaton state");
      }
    }

    /* New pair reached */
    xbt_dynar_push(reached_pairs, &new_pair); 
    
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
 
    compare = 0;
    
    return 0;
    
  }
}


void set_pair_reached(xbt_state_t st){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);
 
  MC_SET_RAW_MEM;

  mc_pair_reached_t pair = NULL;
  pair = xbt_new0(s_mc_pair_reached_t, 1);
  pair->nb = xbt_dynar_length(reached_pairs) + 1;
  pair->automaton_state = st;
  pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  pair->system_state = MC_take_snapshot_liveness();

  /* Get values of propositional symbols */
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  int res;
  int_f_void_t f;

  xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(pair->prop_ato, int, res);
  }

  xbt_dynar_push(reached_pairs, &pair); 
  
  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
    
}

void MC_pair_delete(mc_pair_t pair){
  xbt_free(pair->graph_state->proc_status);
  xbt_free(pair->graph_state);
  xbt_free(pair);
}



int MC_automaton_evaluate_label(xbt_exp_label_t l){
  
  switch(l->type){
  case 0 : {
    int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp);
    int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp);
    return (left_res || right_res);
  }
  case 1 : {
    int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp);
    int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp);
    return (left_res && right_res);
  }
  case 2 : {
    int res = MC_automaton_evaluate_label(l->u.exp_not);
    return (!res);
  }
  case 3 : { 
    unsigned int cursor = 0;
    xbt_propositional_symbol_t p = NULL;
    int_f_void_t f;
    xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, p){
      if(strcmp(p->pred, l->u.predicat) == 0){
        f = (int_f_void_t)p->function;
        return (*f)();
      }
    }
    return -1;
  }
  case 4 : {
    return 2;
  }
  default : 
    return -1;
  }
}


/********************* Double-DFS stateless *******************/

void pair_stateless_free(mc_pair_stateless_t pair){
  xbt_free(pair->graph_state->system_state);
  xbt_free(pair->graph_state->proc_status);
  xbt_free(pair->graph_state);
  xbt_free(pair);
}

void pair_stateless_free_voidp(void *p){
  pair_stateless_free((mc_pair_stateless_t) * (void **) p);
}

mc_pair_stateless_t new_pair_stateless(mc_state_t sg, xbt_state_t st, int r){
  mc_pair_stateless_t p = NULL;
  p = xbt_new0(s_mc_pair_stateless_t, 1);
  p->automaton_state = st;
  p->graph_state = sg;
  p->requests = r;
  mc_stats_pair->expanded_pairs++;
  return p;
}

void pair_reached_free(mc_pair_reached_t pair){
  if(pair){
    pair->automaton_state = NULL;
    xbt_dynar_free(&(pair->prop_ato));
    MC_free_snapshot(pair->system_state);
    xbt_free(pair);
  }
}

void pair_reached_free_voidp(void *p){
  pair_reached_free((mc_pair_reached_t) * (void **) p);
}

void MC_ddfs_init(void){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  XBT_INFO("**************************************************");
  XBT_INFO("Double-DFS init");
  XBT_INFO("**************************************************");

  mc_pair_stateless_t mc_initial_pair = NULL;
  mc_state_t initial_graph_state = NULL;
  smx_process_t process; 

 
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_graph_state = MC_state_pair_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  reached_pairs = xbt_dynar_new(sizeof(mc_pair_reached_t), pair_reached_free_voidp);
  successors = xbt_dynar_new(sizeof(mc_pair_stateless_t), NULL);

  /* Save the initial state */
  initial_state_liveness = xbt_new0(s_mc_global_t, 1);
  initial_state_liveness->initial_snapshot = MC_take_snapshot_liveness();

  MC_UNSET_RAW_MEM; 
  
  unsigned int cursor = 0;
  xbt_state_t state;

  xbt_dynar_foreach(_mc_property_automaton->states, cursor, state){
    if(state->type == -1){
      
      MC_SET_RAW_MEM;
      mc_initial_pair = new_pair_stateless(initial_graph_state, state, MC_state_interleave_size(initial_graph_state));
      xbt_fifo_unshift(mc_stack_liveness, mc_initial_pair);
      MC_UNSET_RAW_MEM;
      
      if(cursor != 0){
        MC_restore_snapshot(initial_state_liveness->initial_snapshot);
        MC_UNSET_RAW_MEM;
      }

      MC_ddfs(0);

    }else{
      if(state->type == 2){
      
        MC_SET_RAW_MEM;
        mc_initial_pair = new_pair_stateless(initial_graph_state, state, MC_state_interleave_size(initial_graph_state));
        xbt_fifo_unshift(mc_stack_liveness, mc_initial_pair);
        MC_UNSET_RAW_MEM;

        set_pair_reached(state);

        if(cursor != 0){
          MC_restore_snapshot(initial_state_liveness->initial_snapshot);
          MC_UNSET_RAW_MEM;
        }
  
        MC_ddfs(1);
  
      }
    }
  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}


void MC_ddfs(int search_cycle){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  smx_process_t process;
  mc_pair_stateless_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness) == 0)
    return;


  /* Get current pair */
  current_pair = (mc_pair_stateless_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness));

  /* Update current state in buchi automaton */
  _mc_property_automaton->current_state = current_pair->automaton_state;

 
  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness), search_cycle);
 
  mc_stats_pair->visited_pairs++;

  //sleep(1);

  int value;
  mc_state_t next_graph_state = NULL;
  smx_simcall_t req = NULL;
  char *req_str;

  xbt_transition_t transition_succ;
  unsigned int cursor = 0;
  int res;

  mc_pair_stateless_t next_pair = NULL;
  mc_pair_stateless_t pair_succ;

  mc_pair_stateless_t remove_pair;
  mc_pair_reached_t remove_pair_reached;
  
  if(xbt_fifo_size(mc_stack_liveness) < MAX_DEPTH_LIVENESS){

    if(current_pair->requests > 0){

      while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){
   
        /* Debug information */
       
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
        xbt_free(req_str);

        MC_state_set_executed_request(current_pair->graph_state, req, value);   

        /* Answer the request */
        SIMIX_simcall_pre(req, value);

        /* Wait for requests (schedules processes) */
        MC_wait_for_requests();

        MC_SET_RAW_MEM;

        /* Create the new expanded graph_state */
        next_graph_state = MC_state_pair_new();

        /* Get enabled process and insert it in the interleave set of the next graph_state */
        xbt_swag_foreach(process, simix_global->process_list){
          if(MC_process_is_enabled(process)){
            MC_state_interleave_process(next_graph_state, process);
          }
        }

        xbt_dynar_reset(successors);

        MC_UNSET_RAW_MEM;


        cursor= 0;
        xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

          res = MC_automaton_evaluate_label(transition_succ->label);

          if(res == 1){ // enabled transition in automaton
            MC_SET_RAW_MEM;
            next_pair = new_pair_stateless(next_graph_state, transition_succ->dst, MC_state_interleave_size(next_graph_state));
            xbt_dynar_push(successors, &next_pair);
            MC_UNSET_RAW_MEM;
          }

        }

        cursor = 0;
   
        xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
          res = MC_automaton_evaluate_label(transition_succ->label);
  
          if(res == 2){ // true transition in automaton
            MC_SET_RAW_MEM;
            next_pair = new_pair_stateless(next_graph_state, transition_succ->dst, MC_state_interleave_size(next_graph_state));
            xbt_dynar_push(successors, &next_pair);
            MC_UNSET_RAW_MEM;
          }

        }

        cursor = 0; 
  
        xbt_dynar_foreach(successors, cursor, pair_succ){

          if(search_cycle == 1){

            if((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2)){ 
          
              if(reached(pair_succ->automaton_state)){
        
                XBT_INFO("Next pair (depth = %d, %u interleave) already reached !", xbt_fifo_size(mc_stack_liveness) + 1, MC_state_interleave_size(pair_succ->graph_state));

                XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
                XBT_INFO("|             ACCEPTANCE CYCLE            |");
                XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
                XBT_INFO("Counter-example that violates formula :");
                MC_show_stack_liveness(mc_stack_liveness);
                MC_dump_stack_liveness(mc_stack_liveness);
                MC_print_statistics_pairs(mc_stats_pair);
                xbt_abort();

              }else{

                XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair (%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->automaton_state->id);

                XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));

                MC_SET_RAW_MEM;
                xbt_fifo_unshift(mc_stack_liveness, pair_succ);
                MC_UNSET_RAW_MEM;
    
                MC_ddfs(search_cycle);

              }

            }else{

              MC_SET_RAW_MEM;
              xbt_fifo_unshift(mc_stack_liveness, pair_succ);
              MC_UNSET_RAW_MEM;
              
              MC_ddfs(search_cycle);
               
            }

          }else{
    
            if(((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

              XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair (%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->automaton_state->id);
      
              set_pair_reached(pair_succ->automaton_state); 

              search_cycle = 1;

              XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));

            }

            MC_SET_RAW_MEM;
            xbt_fifo_unshift(mc_stack_liveness, pair_succ);
            MC_UNSET_RAW_MEM;
            
            MC_ddfs(search_cycle);
           
          }

   
          /* Restore system before checking others successors */
          if(cursor != (xbt_dynar_length(successors) - 1))
            MC_replay_liveness(mc_stack_liveness, 1);
  
    
        }

        if(MC_state_interleave_size(current_pair->graph_state) > 0){
          XBT_DEBUG("Backtracking to depth %d", xbt_fifo_size(mc_stack_liveness));
          MC_replay_liveness(mc_stack_liveness, 0);
        }
      }

 
    }
    
  }else{
    
    XBT_DEBUG("Max depth reached");

  }

  if(xbt_fifo_size(mc_stack_liveness) == MAX_DEPTH_LIVENESS ){
    XBT_DEBUG("Pair (depth = %d) shifted in stack, maximum depth reached", xbt_fifo_size(mc_stack_liveness) );
  }else{
    XBT_DEBUG("Pair (depth = %d) shifted in stack", xbt_fifo_size(mc_stack_liveness) );
  }

  
  MC_SET_RAW_MEM;
  remove_pair = xbt_fifo_shift(mc_stack_liveness);
  xbt_fifo_remove(mc_stack_liveness, remove_pair);
  remove_pair = NULL;
  if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){
    remove_pair_reached = xbt_dynar_pop_as(reached_pairs, mc_pair_reached_t);
    pair_reached_free(remove_pair_reached);
    remove_pair_reached = NULL;
  }
  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;

}
