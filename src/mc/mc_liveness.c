/* Copyright (c) 2011-2013 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"
#include <unistd.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

/********* Global variables *********/

xbt_dynar_t acceptance_pairs;
xbt_dynar_t visited_pairs;
xbt_dynar_t successors;


/********* Static functions *********/

static xbt_dynar_t get_atomic_propositions_values(){
  int res;
  int_f_void_t f;
  unsigned int cursor = 0;
  xbt_automaton_propositional_symbol_t ps = NULL;
  xbt_dynar_t values = xbt_dynar_new(sizeof(int), NULL);

  xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = f();
    xbt_dynar_push_as(values, int, res);
  }

  return values;
}

static int is_reached_acceptance_pair(mc_pair_t pair){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);
 
  if(xbt_dynar_is_empty(acceptance_pairs)){

    MC_SET_RAW_MEM;
    if(pair->graph_state->system_state == NULL){
      pair->graph_state->system_state = MC_take_snapshot();
      pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
    }
    xbt_dynar_push(acceptance_pairs, &pair); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;

    return -1;

  }else{

    MC_SET_RAW_MEM;

    if(pair->graph_state->system_state == NULL){
      pair->graph_state->system_state = MC_take_snapshot();
      pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
    }

    size_t current_bytes_used = pair->heap_bytes_used;
    int current_nb_processes = pair->nb_processes;

    unsigned int cursor = 0;
    int previous_cursor = 0, next_cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(acceptance_pairs) - 1;

    mc_pair_t pair_test = NULL;
    size_t bytes_used_test;
    int nb_processes_test;
    int same_processes_and_bytes_not_found = 1;

    while(start <= end && same_processes_and_bytes_not_found){
      cursor = (start + end) / 2;
      pair_test = (mc_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_pair_t);
      bytes_used_test = pair_test->heap_bytes_used;
      nb_processes_test = pair_test->nb_processes;
      if(nb_processes_test < current_nb_processes)
        start = cursor + 1;
      if(nb_processes_test > current_nb_processes)
        end = cursor - 1; 
      if(nb_processes_test == current_nb_processes){
        if(bytes_used_test < current_bytes_used)
          start = cursor + 1;
        if(bytes_used_test > current_bytes_used)
          end = cursor - 1;
        if(bytes_used_test == current_bytes_used){
          same_processes_and_bytes_not_found = 0;
          if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
            if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){
              if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                if(raw_mem_set)
                  MC_SET_RAW_MEM;
                else
                  MC_UNSET_RAW_MEM;
                return pair_test->num;
              }
            }
          }
          /* Search another pair with same number of bytes used in std_heap */
          previous_cursor = cursor - 1;
          while(previous_cursor >= 0){
            pair_test = (mc_pair_t)xbt_dynar_get_as(acceptance_pairs, previous_cursor, mc_pair_t);
            bytes_used_test = pair_test->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){  
                if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                  if(raw_mem_set)
                    MC_SET_RAW_MEM;
                  else
                    MC_UNSET_RAW_MEM;
                  return pair_test->num;
                }
              }
            }
            previous_cursor--;
          }
          next_cursor = cursor + 1;
          while(next_cursor < xbt_dynar_length(acceptance_pairs)){
            pair_test = (mc_pair_t)xbt_dynar_get_as(acceptance_pairs, next_cursor, mc_pair_t);
            bytes_used_test = pair_test->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){
                if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                  if(raw_mem_set)
                    MC_SET_RAW_MEM;
                  else
                    MC_UNSET_RAW_MEM;
                  return pair_test->num;
                }
              }
            }
            next_cursor++;
          }
        }
      }
    }

    pair_test = (mc_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_pair_t);
    bytes_used_test = pair_test->heap_bytes_used;

    if(bytes_used_test < current_bytes_used)
      xbt_dynar_insert_at(acceptance_pairs, cursor + 1, &pair);
    else
      xbt_dynar_insert_at(acceptance_pairs, cursor, &pair);
       
    
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
    
    return -1;
    
  }
 
}


static void set_acceptance_pair_reached(mc_pair_t pair){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  if(xbt_dynar_is_empty(acceptance_pairs)){

     MC_SET_RAW_MEM;
     if(pair->graph_state->system_state == NULL){
       pair->graph_state->system_state = MC_take_snapshot();
       pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
     }
     xbt_dynar_push(acceptance_pairs, &pair); 
     MC_UNSET_RAW_MEM;

  }else{

    MC_SET_RAW_MEM;

    if(pair->graph_state->system_state == NULL){
      pair->graph_state->system_state = MC_take_snapshot();
      pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
    }
    
    size_t current_bytes_used = pair->heap_bytes_used;
    int current_nb_processes = pair->nb_processes;

    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(acceptance_pairs) - 1;

    mc_pair_t pair_test = NULL;
    size_t bytes_used_test = 0;
    int nb_processes_test;

    while(start <= end){
      cursor = (start + end) / 2;
      pair_test = (mc_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_pair_t);
      bytes_used_test = pair_test->heap_bytes_used;
      nb_processes_test = pair_test->nb_processes;
      if(nb_processes_test < current_nb_processes)
        start = cursor + 1;
      if(nb_processes_test > current_nb_processes)
        end = cursor - 1; 
      if(nb_processes_test == current_nb_processes){
        if(bytes_used_test < current_bytes_used)
          start = cursor + 1;
        if(bytes_used_test > current_bytes_used)
          end = cursor - 1;
        if(bytes_used_test == current_bytes_used)
          break;
      }
    }

    if(bytes_used_test < current_bytes_used)
      xbt_dynar_insert_at(acceptance_pairs, cursor + 1, &pair);
    else
      xbt_dynar_insert_at(acceptance_pairs, cursor, &pair);
    
    MC_UNSET_RAW_MEM;
    
  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
    
}

static void remove_acceptance_pair(mc_pair_t pair){

  unsigned int cursor = 0;
  mc_pair_t pair_test;
  int pair_found = 0;

  xbt_dynar_foreach(acceptance_pairs, cursor, pair_test){
    if(pair_test->num == pair->num){
      pair_found = 1;
      break;
    }
  }

  if(pair_found)
    xbt_dynar_remove_at(acceptance_pairs, cursor, NULL);
  
  pair->acceptance_removed = 1;

  if(pair->stack_removed && pair->acceptance_removed){
    if(_sg_mc_visited == 0){
      MC_pair_delete(pair);
    }else if(pair->visited_removed){
      MC_pair_delete(pair);
    }
  }

}

static int is_visited_pair(mc_pair_t pair){

  if(_sg_mc_visited == 0)
    return -1;

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  if(xbt_dynar_is_empty(visited_pairs)){

    MC_SET_RAW_MEM;
    if(pair->graph_state->system_state == NULL)
      pair->graph_state->system_state = MC_take_snapshot();
    xbt_dynar_push(visited_pairs, &pair); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;

    return -1;

  }else{

    MC_SET_RAW_MEM;

    if(pair->graph_state->system_state == NULL)
      pair->graph_state->system_state = MC_take_snapshot();
    
    size_t current_bytes_used = pair->heap_bytes_used;
    int current_nb_processes = pair->nb_processes;

    unsigned int cursor = 0;
    int previous_cursor = 0, next_cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(visited_pairs) - 1;

    mc_pair_t pair_test = NULL;
    size_t bytes_used_test;
    int nb_processes_test;
    int same_processes_and_bytes_not_found = 1;
    int result;

    while(start <= end && same_processes_and_bytes_not_found){
      cursor = (start + end) / 2;
      pair_test = (mc_pair_t)xbt_dynar_get_as(visited_pairs, cursor, mc_pair_t);
      bytes_used_test = pair_test->heap_bytes_used;
      nb_processes_test = pair_test->nb_processes;
      if(nb_processes_test < current_nb_processes)
        start = cursor + 1;
      if(nb_processes_test > current_nb_processes)
        end = cursor - 1; 
      if(nb_processes_test == current_nb_processes){
        if(bytes_used_test < current_bytes_used)
          start = cursor + 1;
        if(bytes_used_test > current_bytes_used)
          end = cursor - 1;
        if(bytes_used_test == current_bytes_used){
          same_processes_and_bytes_not_found = 0;
          if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
            if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){
              if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                xbt_dynar_remove_at(visited_pairs, cursor, NULL);
                xbt_dynar_insert_at(visited_pairs, cursor, &pair);
                pair_test->visited_removed = 1;
                result = pair_test->num;
                if(pair_test->stack_removed && pair_test->visited_removed){
                  if((pair_test->automaton_state->type == 1) || (pair_test->automaton_state->type == 2)){
                    if(pair_test->acceptance_removed){ 
                      MC_pair_delete(pair_test);
                    }
                  }else{
                    MC_pair_delete(pair_test);
                  }     
                }
                if(raw_mem_set)
                  MC_SET_RAW_MEM;
                else
                  MC_UNSET_RAW_MEM;
                return result;
              }
            }
          }
          /* Search another pair with same number of bytes used in std_heap */
          previous_cursor = cursor - 1;
          while(previous_cursor >= 0){
            pair_test = (mc_pair_t)xbt_dynar_get_as(visited_pairs, previous_cursor, mc_pair_t);
            bytes_used_test = pair_test->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){  
                if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                  xbt_dynar_remove_at(visited_pairs, previous_cursor, NULL);
                  xbt_dynar_insert_at(visited_pairs, previous_cursor, &pair);
                  pair_test->visited_removed = 1;
                  result = pair_test->num;
                  if(pair_test->stack_removed && pair_test->visited_removed){
                    if((pair_test->automaton_state->type == 1) || (pair_test->automaton_state->type == 2)){
                      if(pair_test->acceptance_removed){ 
                        MC_pair_delete(pair_test);
                      }
                    }else{
                      MC_pair_delete(pair_test);
                    }     
                  }
                  if(raw_mem_set)
                    MC_SET_RAW_MEM;
                  else
                    MC_UNSET_RAW_MEM;
                  return result;
                }
              }
            }
            previous_cursor--;
          }
          next_cursor = cursor + 1;
          while(next_cursor < xbt_dynar_length(visited_pairs)){
            pair_test = (mc_pair_t)xbt_dynar_get_as(visited_pairs, next_cursor, mc_pair_t);
            bytes_used_test = pair_test->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->atomic_propositions, pair->atomic_propositions) == 0){
                if(snapshot_compare(pair->graph_state->system_state, pair_test->graph_state->system_state) == 0){
                  xbt_dynar_remove_at(visited_pairs, next_cursor, NULL);
                  xbt_dynar_insert_at(visited_pairs, next_cursor, &pair);
                  pair_test->visited_removed = 1;
                  result = pair_test->num;
                  if(pair_test->stack_removed && pair_test->visited_removed){
                    if((pair_test->automaton_state->type == 1) || (pair_test->automaton_state->type == 2)){
                      if(pair_test->acceptance_removed){ 
                        MC_pair_delete(pair_test);
                      }
                    }else{
                      MC_pair_delete(pair_test);
                    }     
                  }
                  if(raw_mem_set)
                    MC_SET_RAW_MEM;
                  else
                    MC_UNSET_RAW_MEM;
                  return result;
                }
              }
            }
            next_cursor++;
          }
        }
      }
    }

    pair_test = (mc_pair_t)xbt_dynar_get_as(visited_pairs, cursor, mc_pair_t);
    bytes_used_test = pair_test->heap_bytes_used;

    if(bytes_used_test < current_bytes_used)
      xbt_dynar_insert_at(visited_pairs, cursor + 1, &pair);
    else
      xbt_dynar_insert_at(visited_pairs, cursor, &pair);

    if(xbt_dynar_length(visited_pairs) > _sg_mc_visited){
      int min = mc_stats->expanded_states;
      unsigned int cursor2 = 0;
      unsigned int index = 0;
      xbt_dynar_foreach(visited_pairs, cursor2, pair_test){
        if(pair_test->num < min){
          index = cursor2;
          min = pair_test->num;
        }
      }
      xbt_dynar_remove_at(visited_pairs, index, &pair_test);
      pair_test->visited_removed = 1;
      if(pair_test->stack_removed && pair_test->acceptance_removed && pair_test->visited_removed)
        MC_pair_delete(pair_test);
      
    }

    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
    
    return -1;
    
  }
 
}

static int MC_automaton_evaluate_label(xbt_automaton_exp_label_t l, xbt_dynar_t atomic_propositions_values){
  
  switch(l->type){
  case 0 : {
    int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp, atomic_propositions_values);
    int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp, atomic_propositions_values);
    return (left_res || right_res);
  }
  case 1 : {
    int left_res = MC_automaton_evaluate_label(l->u.or_and.left_exp, atomic_propositions_values);
    int right_res = MC_automaton_evaluate_label(l->u.or_and.right_exp, atomic_propositions_values);
    return (left_res && right_res);
  }
  case 2 : {
    int res = MC_automaton_evaluate_label(l->u.exp_not, atomic_propositions_values);
    return (!res);
  }
  case 3 : { 
    unsigned int cursor = 0;
    xbt_automaton_propositional_symbol_t p = NULL;
    xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, p){
      if(strcmp(p->pred, l->u.predicat) == 0)
        return (int)xbt_dynar_get_as(atomic_propositions_values, cursor, int);
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


/********* DDFS Algorithm *********/


void MC_ddfs_init(void){

  initial_state_liveness->raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS init");
  XBT_DEBUG("**************************************************");

  mc_pair_t initial_pair = NULL;
  smx_process_t process; 

  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  acceptance_pairs = xbt_dynar_new(sizeof(mc_pair_t), NULL); 
  visited_pairs = xbt_dynar_new(sizeof(mc_pair_t), NULL); 
  successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);

  initial_state_liveness->snapshot = MC_take_snapshot();

  MC_UNSET_RAW_MEM; 
  
  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;

  xbt_dynar_foreach(_mc_property_automaton->states, cursor, automaton_state){
    if(automaton_state->type == -1){ /* Initial automaton state */
      
      MC_SET_RAW_MEM;

      initial_pair = MC_pair_new();
      initial_pair->automaton_state = automaton_state;
      initial_pair->graph_state = MC_state_new();
      initial_pair->atomic_propositions = get_atomic_propositions_values();

      /* Get enabled process and insert it in the interleave set of the graph_state */
      xbt_swag_foreach(process, simix_global->process_list){
        if(MC_process_is_enabled(process)){
          MC_state_interleave_process(initial_pair->graph_state, process);
        }
      }

      initial_pair->requests = MC_state_interleave_size(initial_pair->graph_state);
      initial_pair->search_cycle = 0;

      xbt_fifo_unshift(mc_stack_liveness, initial_pair);

      MC_UNSET_RAW_MEM;
      
      MC_ddfs();
      
      if(cursor != 0){
        MC_restore_snapshot(initial_state_liveness->snapshot);
        MC_UNSET_RAW_MEM;
      }
      

    }else if(automaton_state->type == 2){ /* Acceptance automaton state */
      
      MC_SET_RAW_MEM;

      initial_pair = MC_pair_new();
      initial_pair->automaton_state = automaton_state;
      initial_pair->graph_state = MC_state_new();
      initial_pair->atomic_propositions = get_atomic_propositions_values();
        
      /* Get enabled process and insert it in the interleave set of the graph_state */
      xbt_swag_foreach(process, simix_global->process_list){
        if(MC_process_is_enabled(process)){
          MC_state_interleave_process(initial_pair->graph_state, process);
        }
      }

      initial_pair->requests = MC_state_interleave_size(initial_pair->graph_state);
      initial_pair->search_cycle = 1;
        
      xbt_fifo_unshift(mc_stack_liveness, initial_pair);
        
      MC_UNSET_RAW_MEM;

      set_acceptance_pair_reached(initial_pair);
  
      MC_ddfs();

      if(cursor != 0){
        MC_restore_snapshot(initial_state_liveness->snapshot);
        MC_UNSET_RAW_MEM;
      }
    }
  }

  if(initial_state_liveness->raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}


void MC_ddfs(){

  smx_process_t process;
  mc_pair_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness) == 0)
    return;

  /* Get current pair */
  current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness));

  /* Update current state in buchi automaton */
  _mc_property_automaton->current_state = current_pair->automaton_state;

  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness), current_pair->search_cycle);
 
  mc_stats->visited_pairs++;

  int value;
  smx_simcall_t req = NULL;
  char *req_str;

  xbt_automaton_transition_t transition_succ;
  unsigned int cursor = 0;
  int res;
  int reached_num, visited_num;
  int new_pair = 0;

  mc_pair_t next_pair = NULL;
  xbt_dynar_t prop_values = NULL;
  
  if(xbt_fifo_size(mc_stack_liveness) < _sg_mc_max_depth){

    if(current_pair->requests > 0){

      if(current_pair->search_cycle){

        if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){ 
          
          if((reached_num = is_reached_acceptance_pair(current_pair)) != -1){
        
            XBT_INFO("Pair %d already reached (equal to pair %d) !", current_pair->num, reached_num);
            
            MC_SET_RAW_MEM;
            xbt_fifo_shift(mc_stack_liveness);
            MC_UNSET_RAW_MEM;

            XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
            XBT_INFO("|             ACCEPTANCE CYCLE            |");
            XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
            XBT_INFO("Counter-example that violates formula :");
            MC_show_stack_liveness(mc_stack_liveness);
            MC_dump_stack_liveness(mc_stack_liveness);
            MC_print_statistics(mc_stats);
            xbt_abort();

          }
        }
      }

      if((visited_num = is_visited_pair(current_pair)) != -1){

        XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", current_pair->num, visited_num);
      
      }else{            

        while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){
   
          /* Debug information */
          
          if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Execute: %s", req_str);
            xbt_free(req_str);
          }
          
          MC_state_set_executed_request(current_pair->graph_state, req, value);  
          mc_stats->executed_transitions++;

          /* Answer the request */
          SIMIX_simcall_pre(req, value);
          
          /* Wait for requests (schedules processes) */
          MC_wait_for_requests();

          MC_SET_RAW_MEM;
          prop_values = get_atomic_propositions_values();
          MC_UNSET_RAW_MEM;
         
          /* Evaluate enabled transition according to atomic propositions values */
          cursor= 0;
          xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

            res = MC_automaton_evaluate_label(transition_succ->label, prop_values);

            if(res == 1){ // enabled transition in automaton

              if(new_pair)
               MC_replay_liveness(mc_stack_liveness, 1); 

              MC_SET_RAW_MEM;

              next_pair = MC_pair_new();
              next_pair->graph_state = MC_state_new();
              next_pair->automaton_state = transition_succ->dst;
              next_pair->atomic_propositions = get_atomic_propositions_values();

              /* Get enabled process and insert it in the interleave set of the next graph_state */
              xbt_swag_foreach(process, simix_global->process_list){
                if(MC_process_is_enabled(process)){
                  MC_state_interleave_process(next_pair->graph_state, process);
                }
              }

              next_pair->requests = MC_state_interleave_size(next_pair->graph_state);
              
              if(next_pair->automaton_state->type == 1 || next_pair->automaton_state->type == 2 || current_pair->search_cycle)
                next_pair->search_cycle = 1;
            
              xbt_fifo_unshift(mc_stack_liveness, next_pair);

              if(mc_stats->expanded_pairs%1000000 == 0)
                XBT_INFO("Expanded pairs : %lu", mc_stats->expanded_pairs);

              MC_UNSET_RAW_MEM;

              new_pair = 1;

              MC_ddfs();

            }

          }

          /* Then, evaluate true transitions (always true, whatever atomic propositions values) */
          cursor = 0;   
          xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
            res = MC_automaton_evaluate_label(transition_succ->label, prop_values);
  
            if(res == 2){ // true transition in automaton

              if(new_pair)
                MC_replay_liveness(mc_stack_liveness, 1); 
            
              MC_SET_RAW_MEM;
            
              next_pair = MC_pair_new();
              next_pair->graph_state = MC_state_new();
              next_pair->automaton_state = transition_succ->dst;
              next_pair->atomic_propositions = get_atomic_propositions_values();

              /* Get enabled process and insert it in the interleave set of the next graph_state */
              xbt_swag_foreach(process, simix_global->process_list){
                if(MC_process_is_enabled(process)){
                  MC_state_interleave_process(next_pair->graph_state, process);
                }
              }

              next_pair->requests = MC_state_interleave_size(next_pair->graph_state);
            
              if(next_pair->automaton_state->type == 1 || next_pair->automaton_state->type == 2 || current_pair->search_cycle)
                next_pair->search_cycle = 1;

              xbt_fifo_unshift(mc_stack_liveness, next_pair);

              if(mc_stats->expanded_pairs%1000000 == 0)
                XBT_INFO("Expanded pairs : %lu", mc_stats->expanded_pairs);
            
              MC_UNSET_RAW_MEM;

              new_pair = 1;

              MC_ddfs();

            }

          }

          if(MC_state_interleave_size(current_pair->graph_state) > 0){
            XBT_DEBUG("Backtracking to depth %d", xbt_fifo_size(mc_stack_liveness));
            MC_replay_liveness(mc_stack_liveness, 0);
          }
        
        }

      }
 
    }else{

      mc_stats->executed_transitions++;
      
      XBT_DEBUG("No request to execute in this state, search evolution in Büchi Automaton.");

      if(current_pair->search_cycle){

        if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){ 
          
          if((reached_num = is_reached_acceptance_pair(current_pair)) != -1){
        
            XBT_INFO("Pair %d already reached (equal to pair %d) !", current_pair->num, reached_num);
            
            MC_SET_RAW_MEM;
            xbt_fifo_shift(mc_stack_liveness);
            MC_UNSET_RAW_MEM;

            XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
            XBT_INFO("|             ACCEPTANCE CYCLE            |");
            XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
            XBT_INFO("Counter-example that violates formula :");
            MC_show_stack_liveness(mc_stack_liveness);
            MC_dump_stack_liveness(mc_stack_liveness);
            MC_print_statistics(mc_stats);
            xbt_abort();

          }
        }
      }

      if((visited_num = is_visited_pair(current_pair)) != -1){

        XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", current_pair->num, visited_num);
      
      }else{            
        
        MC_SET_RAW_MEM;
        prop_values = get_atomic_propositions_values();
        MC_UNSET_RAW_MEM;

        /* Evaluate enabled transition according to atomic propositions values */
        cursor= 0;
        xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

          res = MC_automaton_evaluate_label(transition_succ->label, prop_values);

          if(res == 1){ // enabled transition in automaton

            if(new_pair)
              MC_replay_liveness(mc_stack_liveness, 1);

            MC_SET_RAW_MEM;

            next_pair = MC_pair_new();
            next_pair->graph_state = MC_state_new();
            next_pair->automaton_state = transition_succ->dst;
            next_pair->atomic_propositions = get_atomic_propositions_values();
            next_pair->requests = MC_state_interleave_size(next_pair->graph_state);
              
            if(next_pair->automaton_state->type == 1 || next_pair->automaton_state->type == 2 || current_pair->search_cycle)
              next_pair->search_cycle = 1;
            
            xbt_fifo_unshift(mc_stack_liveness, next_pair);

            if(mc_stats->expanded_pairs%1000 == 0)
              XBT_INFO("Expanded pairs : %lu", mc_stats->expanded_pairs);

            MC_UNSET_RAW_MEM;

            new_pair = 1;

            MC_ddfs();

          }

        }

        /* Then, evaluate true transitions (always true, whatever atomic propositions values) */
        cursor = 0;   
        xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
          res = MC_automaton_evaluate_label(transition_succ->label, prop_values);
  
          if(res == 2){ // true transition in automaton

            if(new_pair)
              MC_replay_liveness(mc_stack_liveness, 1);

            MC_SET_RAW_MEM;
            
            next_pair = MC_pair_new();
            next_pair->graph_state = MC_state_new();
            next_pair->automaton_state = transition_succ->dst;
            next_pair->atomic_propositions = get_atomic_propositions_values();
            next_pair->requests = MC_state_interleave_size(next_pair->graph_state);
            
            if(next_pair->automaton_state->type == 1 || next_pair->automaton_state->type == 2 || current_pair->search_cycle)
              next_pair->search_cycle = 1;

            xbt_fifo_unshift(mc_stack_liveness, next_pair);

            if(mc_stats->expanded_pairs%1000 == 0)
              XBT_INFO("Expanded pairs : %lu", mc_stats->expanded_pairs);
            
            MC_UNSET_RAW_MEM;

            new_pair = 1;

            MC_ddfs();

          }

        }
      }
    }
    
  }else{
    
    XBT_WARN("/!\\ Max depth reached ! /!\\ ");
    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      XBT_WARN("/!\\ But, there are still processes to interleave. Model-checker will not be able to ensure the soundness of the verification from now. /!\\ "); 
      if(_sg_mc_max_depth == 1000)
        XBT_WARN("Notice : the default value of max depth is 1000 but you can change it with cfg=model-check/max_depth:value.");
    }
    
  }

  if(xbt_fifo_size(mc_stack_liveness) == _sg_mc_max_depth ){
    XBT_DEBUG("Pair %d (depth = %d) shifted in stack, maximum depth reached", current_pair->num, xbt_fifo_size(mc_stack_liveness) );
  }else{
    XBT_DEBUG("Pair %d (depth = %d) shifted in stack", current_pair->num, xbt_fifo_size(mc_stack_liveness) );
  }

  
  MC_SET_RAW_MEM;
  if(prop_values != NULL)
    xbt_dynar_free(&prop_values);
  current_pair = xbt_fifo_shift(mc_stack_liveness);
  current_pair->stack_removed = 1;
  if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){
    remove_acceptance_pair(current_pair);
  }else{
    if(_sg_mc_visited == 0)
      MC_pair_delete(current_pair);
    else if(current_pair->visited_removed)
      MC_pair_delete(current_pair);
  }
  MC_UNSET_RAW_MEM;

}
