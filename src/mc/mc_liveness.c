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

static mc_acceptance_pair_t acceptance_pair_new(int num, xbt_automaton_state_t as){

  mc_acceptance_pair_t new_pair = NULL;
  new_pair = xbt_new0(s_mc_acceptance_pair_t, 1);
  new_pair->num = num;
  new_pair->automaton_state = as;
  new_pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  new_pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  new_pair->nb_processes = xbt_swag_size(simix_global->process_list);
  new_pair->system_state = MC_take_snapshot();  
  
  /* Get values of propositional symbols */
  int res;
  int_f_void_t f;
  unsigned int cursor = 0;
  xbt_automaton_propositional_symbol_t ps = NULL;
  xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = f();
    xbt_dynar_push_as(new_pair->prop_ato, int, res);
  }

  return new_pair;

}

static int is_reached_acceptance_pair(int num, xbt_automaton_state_t as){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;
  mc_acceptance_pair_t new_pair = acceptance_pair_new(num, as);  
  MC_UNSET_RAW_MEM;
  
  if(xbt_dynar_is_empty(acceptance_pairs)){

    MC_SET_RAW_MEM;
    xbt_dynar_push(acceptance_pairs, &new_pair); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;

    return -1;

  }else{

    MC_SET_RAW_MEM;
    
    size_t current_bytes_used = new_pair->heap_bytes_used;
    int current_nb_processes = new_pair->nb_processes;

    unsigned int cursor = 0;
    int previous_cursor = 0, next_cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(acceptance_pairs) - 1;

    mc_acceptance_pair_t pair_test = NULL;
    size_t bytes_used_test;
    int nb_processes_test;
    int same_processes_and_bytes_not_found = 1;

    while(start <= end && same_processes_and_bytes_not_found){
      cursor = (start + end) / 2;
      pair_test = (mc_acceptance_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_acceptance_pair_t);
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
          if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
            if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){
              if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
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
            pair_test = (mc_acceptance_pair_t)xbt_dynar_get_as(acceptance_pairs, previous_cursor, mc_acceptance_pair_t);
            bytes_used_test = pair_test->system_state->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){  
                if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
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
            pair_test = (mc_acceptance_pair_t)xbt_dynar_get_as(acceptance_pairs, next_cursor, mc_acceptance_pair_t);
            bytes_used_test = pair_test->system_state->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){
                if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
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

    pair_test = (mc_acceptance_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_acceptance_pair_t);
    bytes_used_test = pair_test->heap_bytes_used;

    if(bytes_used_test < current_bytes_used)
      xbt_dynar_insert_at(acceptance_pairs, cursor + 1, &new_pair);
    else
      xbt_dynar_insert_at(acceptance_pairs, cursor, &new_pair);
    
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
    
    return -1;
    
  }
 
}


static void set_acceptance_pair_reached(int num, xbt_automaton_state_t as){

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);
 
  MC_SET_RAW_MEM;
  mc_acceptance_pair_t new_pair = acceptance_pair_new(num, as);
  MC_UNSET_RAW_MEM;

  if(xbt_dynar_is_empty(acceptance_pairs)){

     MC_SET_RAW_MEM;
     xbt_dynar_push(acceptance_pairs, &new_pair); 
     MC_UNSET_RAW_MEM;

  }else{

    MC_SET_RAW_MEM;
    
    size_t current_bytes_used = new_pair->heap_bytes_used;
    int current_nb_processes = new_pair->nb_processes;

    unsigned int cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(acceptance_pairs) - 1;

    mc_acceptance_pair_t pair_test = NULL;
    size_t bytes_used_test;
    int nb_processes_test;

    while(start <= end){
      cursor = (start + end) / 2;
      pair_test = (mc_acceptance_pair_t)xbt_dynar_get_as(acceptance_pairs, cursor, mc_acceptance_pair_t);
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
      xbt_dynar_insert_at(acceptance_pairs, cursor + 1, &new_pair);
    else
      xbt_dynar_insert_at(acceptance_pairs, cursor, &new_pair);
    
    MC_UNSET_RAW_MEM;
    
  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
    
}

static void remove_acceptance_pair(int num_pair){

  unsigned int cursor = 0;
  mc_acceptance_pair_t current_pair;

  xbt_dynar_foreach(acceptance_pairs, cursor, current_pair){
    if(current_pair->num == num_pair)
      break;
  }
  
  xbt_dynar_remove_at(acceptance_pairs, cursor, NULL);

}

static mc_visited_pair_t visited_pair_new(int num, xbt_automaton_state_t as){

  mc_visited_pair_t new_pair = NULL;
  new_pair = xbt_new0(s_mc_visited_pair_t, 1);
  new_pair->automaton_state = as;
  new_pair->num = num;
  new_pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  new_pair->heap_bytes_used = mmalloc_get_bytes_used(std_heap);
  new_pair->nb_processes = xbt_swag_size(simix_global->process_list);
  new_pair->system_state = MC_take_snapshot();  
  
  /* Get values of propositional symbols */
  int res;
  int_f_void_t f;
  unsigned int cursor = 0;
  xbt_automaton_propositional_symbol_t ps = NULL;
  xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = f();
    xbt_dynar_push_as(new_pair->prop_ato, int, res);
  }

  return new_pair;

}

static int is_visited_pair(int num, xbt_automaton_state_t as){

  if(_sg_mc_visited == 0)
    return -1;

  int raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;
  mc_visited_pair_t new_pair = visited_pair_new(num, as);
  MC_UNSET_RAW_MEM;

  if(xbt_dynar_is_empty(visited_pairs)){

    MC_SET_RAW_MEM;
    xbt_dynar_push(visited_pairs, &new_pair); 
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;

    return -1;

  }else{

    MC_SET_RAW_MEM;
    
    size_t current_bytes_used = new_pair->heap_bytes_used;
    int current_nb_processes = new_pair->nb_processes;

    unsigned int cursor = 0;
    int previous_cursor = 0, next_cursor = 0;
    int start = 0;
    int end = xbt_dynar_length(visited_pairs) - 1;

    mc_visited_pair_t pair_test = NULL;
    size_t bytes_used_test;
    int nb_processes_test;
    int same_processes_and_bytes_not_found = 1;

    while(start <= end && same_processes_and_bytes_not_found){
      cursor = (start + end) / 2;
      pair_test = (mc_visited_pair_t)xbt_dynar_get_as(visited_pairs, cursor, mc_visited_pair_t);
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
          if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
            if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){
              if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
                xbt_dynar_remove_at(visited_pairs, cursor, NULL);
                xbt_dynar_insert_at(visited_pairs, cursor, &new_pair);
                XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", new_pair->num, pair_test->num);
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
            pair_test = (mc_visited_pair_t)xbt_dynar_get_as(visited_pairs, previous_cursor, mc_visited_pair_t);
            bytes_used_test = pair_test->system_state->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){  
                if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
                  xbt_dynar_remove_at(visited_pairs, previous_cursor, NULL);
                  xbt_dynar_insert_at(visited_pairs, previous_cursor, &new_pair);
                  XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", new_pair->num, pair_test->num);
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
          while(next_cursor < xbt_dynar_length(visited_pairs)){
            pair_test = (mc_visited_pair_t)xbt_dynar_get_as(visited_pairs, next_cursor, mc_visited_pair_t);
            bytes_used_test = pair_test->system_state->heap_bytes_used;
            if(bytes_used_test != current_bytes_used)
              break;
            if(xbt_automaton_state_compare(pair_test->automaton_state, as) == 0){
              if(xbt_automaton_propositional_symbols_compare_value(pair_test->prop_ato, new_pair->prop_ato) == 0){
                if(snapshot_compare(new_pair->system_state, pair_test->system_state) == 0){
                  xbt_dynar_remove_at(visited_pairs, next_cursor, NULL);
                  xbt_dynar_insert_at(visited_pairs, next_cursor, &new_pair);
                  XBT_DEBUG("Pair %d already visited ! (equal to pair %d)", new_pair->num, pair_test->num);
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

    pair_test = (mc_visited_pair_t)xbt_dynar_get_as(visited_pairs, cursor, mc_visited_pair_t);
    bytes_used_test = pair_test->heap_bytes_used;

    if(bytes_used_test < current_bytes_used)
      xbt_dynar_insert_at(visited_pairs, cursor + 1, &new_pair);
    else
      xbt_dynar_insert_at(visited_pairs, cursor, &new_pair);

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
      xbt_dynar_remove_at(visited_pairs, index, NULL);
    }

    /*cursor = 0;
    xbt_dynar_foreach(visited_pairs, cursor, pair_test){
      fprintf(stderr, "Visited pair %d, nb_processes %d and heap_bytes_used %zu\n", pair_test->num, pair_test->nb_processes, pair_test->heap_bytes_used);
      }*/
    
    MC_UNSET_RAW_MEM;

    if(raw_mem_set)
      MC_SET_RAW_MEM;
    
    return -1;
    
  }
 
}

static int MC_automaton_evaluate_label(xbt_automaton_exp_label_t l){
  
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
    xbt_automaton_propositional_symbol_t p = NULL;
    int_f_void_t f;
    xbt_dynar_foreach(_mc_property_automaton->propositional_symbols, cursor, p){
      if(strcmp(p->pred, l->u.predicat) == 0){
        f = (int_f_void_t)p->function;
        return f();
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


/********* Free functions *********/

static void visited_pair_free(mc_visited_pair_t pair){
  if(pair){
    xbt_dynar_free(&(pair->prop_ato));
    MC_free_snapshot(pair->system_state);
    xbt_free(pair);
  }
}

static void visited_pair_free_voidp(void *p){
  visited_pair_free((mc_visited_pair_t) * (void **) p);
}

static void acceptance_pair_free(mc_acceptance_pair_t pair){
  if(pair){
    xbt_dynar_free(&(pair->prop_ato));
    MC_free_snapshot(pair->system_state);
    xbt_free(pair);
  }
}

static void acceptance_pair_free_voidp(void *p){
  acceptance_pair_free((mc_acceptance_pair_t) * (void **) p);
}


/********* DDFS Algorithm *********/


void MC_ddfs_init(void){

  initial_state_liveness->raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS init");
  XBT_DEBUG("**************************************************");

  mc_pair_t mc_initial_pair = NULL;
  mc_state_t initial_graph_state = NULL;
  smx_process_t process; 

 
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_graph_state = MC_state_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  acceptance_pairs = xbt_dynar_new(sizeof(mc_acceptance_pair_t), acceptance_pair_free_voidp);
  visited_pairs = xbt_dynar_new(sizeof(mc_visited_pair_t), visited_pair_free_voidp);
  successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);

  /* Save the initial state */
  initial_state_liveness->snapshot = MC_take_snapshot();

  MC_UNSET_RAW_MEM; 
  
  unsigned int cursor = 0;
  xbt_automaton_state_t automaton_state;

  xbt_dynar_foreach(_mc_property_automaton->states, cursor, automaton_state){
    if(automaton_state->type == -1){
      
      MC_SET_RAW_MEM;
      mc_initial_pair = MC_pair_new(initial_graph_state, automaton_state, MC_state_interleave_size(initial_graph_state));
      xbt_fifo_unshift(mc_stack_liveness, mc_initial_pair);
      MC_UNSET_RAW_MEM;
      
      if(cursor != 0){
        MC_restore_snapshot(initial_state_liveness->snapshot);
        MC_UNSET_RAW_MEM;
      }

      MC_ddfs(0);

    }else{
      if(automaton_state->type == 2){
      
        MC_SET_RAW_MEM;
        mc_initial_pair = MC_pair_new(initial_graph_state, automaton_state, MC_state_interleave_size(initial_graph_state));
        xbt_fifo_unshift(mc_stack_liveness, mc_initial_pair);
        MC_UNSET_RAW_MEM;

        set_acceptance_pair_reached(mc_initial_pair->num, automaton_state);

        if(cursor != 0){
          MC_restore_snapshot(initial_state_liveness->snapshot);
          MC_UNSET_RAW_MEM;
        }
  
        MC_ddfs(1);
  
      }
    }
  }

  if(initial_state_liveness->raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}


void MC_ddfs(int search_cycle){

  smx_process_t process;
  mc_pair_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness) == 0)
    return;


  /* Get current pair */
  current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness));

  /* Update current state in buchi automaton */
  _mc_property_automaton->current_state = current_pair->automaton_state;

 
  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness), search_cycle);
 
  mc_stats->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_simcall_t req = NULL;
  char *req_str;

  xbt_automaton_transition_t transition_succ;
  unsigned int cursor = 0;
  int res;

  mc_pair_t next_pair = NULL;
  mc_pair_t pair_succ;

  mc_pair_t pair_to_remove;
  
  if(xbt_fifo_size(mc_stack_liveness) < _sg_mc_max_depth){

    if(current_pair->requests > 0){

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

        /* Create the new expanded graph_state */
        next_graph_state = MC_state_new();

        /* Get enabled process and insert it in the interleave set of the next graph_state */
        xbt_swag_foreach(process, simix_global->process_list){
          if(MC_process_is_enabled(process)){
            XBT_DEBUG("Process %lu enabled with simcall : %d", process->pid, (&process->simcall)->call); 
          }
        }

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
            next_pair = MC_pair_new(next_graph_state, transition_succ->dst, MC_state_interleave_size(next_graph_state));
            xbt_dynar_push(successors, &next_pair);
            MC_UNSET_RAW_MEM;
          }

        }

        cursor = 0;
   
        xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
          res = MC_automaton_evaluate_label(transition_succ->label);
  
          if(res == 2){ // true transition in automaton
            MC_SET_RAW_MEM;
            next_pair = MC_pair_new(next_graph_state, transition_succ->dst,  MC_state_interleave_size(next_graph_state));
            xbt_dynar_push(successors, &next_pair);
            MC_UNSET_RAW_MEM;
          }

        }

        cursor = 0; 
  
        xbt_dynar_foreach(successors, cursor, pair_succ){

          if(search_cycle == 1){

            if((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2)){ 
          
              if(is_reached_acceptance_pair(pair_succ->num, pair_succ->automaton_state) != -1){
        
                XBT_INFO("Next pair (depth = %d, %u interleave) already reached !", xbt_fifo_size(mc_stack_liveness) + 1, MC_state_interleave_size(pair_succ->graph_state));

                XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
                XBT_INFO("|             ACCEPTANCE CYCLE            |");
                XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
                XBT_INFO("Counter-example that violates formula :");
                MC_show_stack_liveness(mc_stack_liveness);
                MC_dump_stack_liveness(mc_stack_liveness);
                MC_print_statistics(mc_stats);
                xbt_abort();

              }else{

                if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){

                  XBT_DEBUG("Next pair already visited !");
                  break;
            
                }else{

                  XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair (%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->automaton_state->id);

                  XBT_DEBUG("Acceptance pairs : %lu", xbt_dynar_length(acceptance_pairs));

                  MC_SET_RAW_MEM;
                  xbt_fifo_unshift(mc_stack_liveness, pair_succ);
                  MC_UNSET_RAW_MEM;
    
                  MC_ddfs(search_cycle);
                
                }

              }

            }else{

              if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){

                XBT_DEBUG("Next pair already visited !");
                break;
                
              }else{

                MC_SET_RAW_MEM;
                xbt_fifo_unshift(mc_stack_liveness, pair_succ);
                MC_UNSET_RAW_MEM;
                
                MC_ddfs(search_cycle);
              }
               
            }

          }else{

            if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){

              XBT_DEBUG("Next pair already visited !");
              break;
            
            }else{
    
              if(((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

                XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair (%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->automaton_state->id);
      
                set_acceptance_pair_reached(pair_succ->num, pair_succ->automaton_state); 

                search_cycle = 1;

                XBT_DEBUG("Acceptance pairs : %lu", xbt_dynar_length(acceptance_pairs));

              }

              MC_SET_RAW_MEM;
              xbt_fifo_unshift(mc_stack_liveness, pair_succ);
              MC_UNSET_RAW_MEM;
            
              MC_ddfs(search_cycle);

            }
           
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

 
    }else{

      mc_stats->executed_transitions++;
      
      XBT_DEBUG("No more request to execute in this state, search evolution in Büchi Automaton.");

      MC_SET_RAW_MEM;

      /* Create the new expanded graph_state */
      next_graph_state = MC_state_new();

      xbt_dynar_reset(successors);

      MC_UNSET_RAW_MEM;


      cursor= 0;
      xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

        res = MC_automaton_evaluate_label(transition_succ->label);

        if(res == 1){ // enabled transition in automaton
          MC_SET_RAW_MEM;
          next_pair = MC_pair_new(next_graph_state, transition_succ->dst, MC_state_interleave_size(next_graph_state));
          xbt_dynar_push(successors, &next_pair);
          MC_UNSET_RAW_MEM;
        }

      }

      cursor = 0;
   
      xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
        res = MC_automaton_evaluate_label(transition_succ->label);
  
        if(res == 2){ // true transition in automaton
          MC_SET_RAW_MEM;
          next_pair = MC_pair_new(next_graph_state, transition_succ->dst, MC_state_interleave_size(next_graph_state));
          xbt_dynar_push(successors, &next_pair);
          MC_UNSET_RAW_MEM;
        }

      }

      cursor = 0; 
     
      xbt_dynar_foreach(successors, cursor, pair_succ){

        if(search_cycle == 1){

          if((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2)){ 

            if(is_reached_acceptance_pair(pair_succ->num, pair_succ->automaton_state) != -1){
           
              XBT_INFO("Next pair (depth = %d) already reached !", xbt_fifo_size(mc_stack_liveness) + 1);
        
              XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
              XBT_INFO("|             ACCEPTANCE CYCLE            |");
              XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
              XBT_INFO("Counter-example that violates formula :");
              MC_show_stack_liveness(mc_stack_liveness);
              MC_dump_stack_liveness(mc_stack_liveness);
              MC_print_statistics(mc_stats);
              xbt_abort();

            }else{

              if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){
                
                XBT_DEBUG("Next pair already visited !");
                break;
                
              }else{

                XBT_INFO("Next pair (depth = %d) -> Acceptance pair (%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->automaton_state->id);
        
                XBT_INFO("Acceptance pairs : %lu", xbt_dynar_length(acceptance_pairs));

                MC_SET_RAW_MEM;
                xbt_fifo_unshift(mc_stack_liveness, pair_succ);
                MC_UNSET_RAW_MEM;
        
                MC_ddfs(search_cycle);
              
              }

            }

          }else{
            
            if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){
              
              XBT_DEBUG("Next pair already visited !");
              break;
              
            }else{

              MC_SET_RAW_MEM;
              xbt_fifo_unshift(mc_stack_liveness, pair_succ);
              MC_UNSET_RAW_MEM;
            
              MC_ddfs(search_cycle);

            }
            
          }
      

        }else{
      
          if(is_visited_pair(pair_succ->num, pair_succ->automaton_state) != -1){

            XBT_DEBUG("Next pair already visited !");
            break;
            
          }else{

            if(((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

              set_acceptance_pair_reached(pair_succ->num, pair_succ->automaton_state);
         
              search_cycle = 1;

              XBT_INFO("Acceptance pairs : %lu", xbt_dynar_length(acceptance_pairs));

            }

            MC_SET_RAW_MEM;
            xbt_fifo_unshift(mc_stack_liveness, pair_succ);
            MC_UNSET_RAW_MEM;
          
            MC_ddfs(search_cycle);
          
          }
          
        }

        /* Restore system before checking others successors */
        if(cursor != xbt_dynar_length(successors) - 1)
          MC_replay_liveness(mc_stack_liveness, 1);

      }           

    }
    
  }else{
    
    XBT_WARN("/!\\ Max depth reached ! /!\\ ");
    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      XBT_WARN("/!\\ But, there are still processes to interleave. Model-checker will not be able to ensure the soundness of the verification from now. /!\\ "); 
      XBT_WARN("Notice : the default value of max depth is 1000 but you can change it with cfg=model-check/max_depth:value.");
    }
    
  }

  if(xbt_fifo_size(mc_stack_liveness) == _sg_mc_max_depth ){
    XBT_DEBUG("Pair (depth = %d) shifted in stack, maximum depth reached", xbt_fifo_size(mc_stack_liveness) );
  }else{
    XBT_DEBUG("Pair (depth = %d) shifted in stack", xbt_fifo_size(mc_stack_liveness) );
  }

  
  MC_SET_RAW_MEM;
  pair_to_remove = xbt_fifo_shift(mc_stack_liveness);
  xbt_fifo_remove(mc_stack_liveness, pair_to_remove);
  pair_to_remove = NULL;
  if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){
    remove_acceptance_pair(current_pair->num);
  }
  MC_UNSET_RAW_MEM;

}
