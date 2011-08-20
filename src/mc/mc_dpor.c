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



/************ DPOR 2 (invisible and independant transitions) ************/

xbt_dynar_t reached_pairs_prop;
xbt_dynar_t visible_transitions;
xbt_dynar_t enabled_processes;

mc_prop_ato_t new_proposition(char* id, int value){
  mc_prop_ato_t prop = NULL;
  prop = xbt_new0(s_mc_prop_ato_t, 1);
  prop->id = strdup(id);
  prop->value = value;
  return prop;
}

mc_pair_prop_t new_pair_prop(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pair_prop_t pair = NULL;
  pair = xbt_new0(s_mc_pair_prop_t, 1);
  pair->system_state = sn;
  pair->automaton_state = st;
  pair->graph_state = sg;
  pair->propositions = xbt_dynar_new(sizeof(mc_prop_ato_t), NULL);
  pair->fully_expanded = 0;
  pair->interleave = 0;
  mc_stats_pair->expanded_pairs++;
  return pair;
}

int reached_prop(mc_pair_prop_t pair){

  char* hash_reached = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&pair, hash);
  xbt_dynar_foreach(reached_pairs_prop, c, hash_reached){
    if(strcmp(hash, hash_reached) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_prop_reached(mc_pair_prop_t pair){

  if(reached_prop(pair) == 0){
    MC_SET_RAW_MEM;
    char *hash = malloc(sizeof(char)*160) ;
    xbt_sha((char *)&pair, hash);
    xbt_dynar_push(reached_pairs_prop, &hash); 
    MC_UNSET_RAW_MEM;
  }

}

int invisible(mc_pair_prop_t p, mc_pair_prop_t np){
  mc_prop_ato_t prop1;
  mc_prop_ato_t prop2;
  int i;
  for(i=0;i<xbt_dynar_length(p->propositions); i++){
    prop1 = xbt_dynar_get_as(p->propositions, i, mc_prop_ato_t);
    prop2 = xbt_dynar_get_as(np->propositions, i, mc_prop_ato_t);
    //XBT_DEBUG("test invisible : prop 1 = %s : %d / prop 2 = %s : %d", prop1->id, prop1->value, prop2->id, prop2->value);
    if(prop1->value != prop2->value)
	  return 0;
  }
  return 1; 

}

void set_fully_expanded(mc_pair_prop_t pair){
  pair->fully_expanded = 1;
}

void MC_dpor2_init(xbt_automaton_t a)
{
  mc_pair_prop_t initial_pair = NULL;
  mc_state_t initial_graph_state = NULL;
  mc_snapshot_t initial_system_state = NULL;
  xbt_state_t initial_automaton_state = NULL;
 

  MC_SET_RAW_MEM;
  initial_graph_state = MC_state_new();
  MC_UNSET_RAW_MEM;

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t automaton_state = NULL;
  int res;
  xbt_transition_t transition_succ;

  xbt_dynar_foreach(a->states, cursor, automaton_state){
    if(automaton_state->type == -1){
      xbt_dynar_foreach(automaton_state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
  	if((res == 1) || (res == 2)){
	  initial_automaton_state = transition_succ->dst;
	  break;
	}
      }
    }

    if(initial_automaton_state != NULL)
      break;
  }

  if(initial_automaton_state == NULL){
    cursor = 0;
    xbt_dynar_foreach(a->states, cursor, automaton_state){
      if(automaton_state->type == -1){
	initial_automaton_state = automaton_state;
	break;
      }
    }
  }

  reached_pairs_prop = xbt_dynar_new(sizeof(char *), NULL); 
  visible_transitions = xbt_dynar_new(sizeof(s_smx_req_t), NULL);
  enabled_processes = xbt_dynar_new(sizeof(smx_process_t), NULL);

  MC_SET_RAW_MEM;
  initial_system_state = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_system_state);
  initial_pair = new_pair_prop(initial_system_state, initial_graph_state, initial_automaton_state);
  cursor = 0;
  xbt_propositional_symbol_t ps;
  xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
    int (*f)() = ps->function;
    int val = (*f)();
    mc_prop_ato_t pa = new_proposition(ps->pred, val);
    xbt_dynar_push(initial_pair->propositions, &pa);
  } 
  xbt_fifo_unshift(mc_stack_liveness_stateful, initial_pair);
  MC_UNSET_RAW_MEM;


  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial pair");

  MC_dpor2(a, 0);
    
}


void MC_dpor2(xbt_automaton_t a, int search_cycle)
{
  char *req_str;
  int value;
  smx_req_t req = NULL, prev_req = NULL;
  mc_state_t next_graph_state = NULL;
  mc_snapshot_t next_system_state = NULL;
  xbt_state_t next_automaton_state = NULL;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;
  mc_pair_prop_t pair = NULL, next_pair = NULL, prev_pair = NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  int d;


  while (xbt_fifo_size(mc_stack_liveness_stateful) > 0) {

    /* Get current pair */
    pair = (mc_pair_prop_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (pair=%p) (%d interleave)",
	      xbt_fifo_size(mc_stack_liveness_stateful), pair, MC_state_interleave_size(pair->graph_state));

    if(MC_state_interleave_size(pair->graph_state) == 0){
    
      xbt_dynar_reset(enabled_processes);

      MC_SET_RAW_MEM;
    
      xbt_swag_foreach(process, simix_global->process_list){
	if(MC_process_is_enabled(process)){ 
	  xbt_dynar_push(enabled_processes, &process);
	  //XBT_DEBUG("Process : pid=%lu",process->pid);
	}
      }

      //XBT_DEBUG("Enabled processes before pop : %lu", xbt_dynar_length(enabled_processes));
    
      //XBT_DEBUG("Processes already interleaved : %d", pair->interleave);
    
      if(xbt_dynar_length(enabled_processes) > 0){
	for(d=0;d<pair->interleave;d++){
	  xbt_dynar_shift(enabled_processes, NULL);
	}
      }

      //XBT_DEBUG("Enabled processes after pop : %lu", xbt_dynar_length(enabled_processes));
    
      if(pair->fully_expanded == 0){
	if(xbt_dynar_length(enabled_processes) > 0){
	  MC_state_interleave_process(pair->graph_state, xbt_dynar_get_as(enabled_processes, 0, smx_process_t));
	  //XBT_DEBUG("Interleave process");
	}
      }
    
      MC_UNSET_RAW_MEM;

    }


    /* Update statistics */
    mc_stats_pair->visited_pairs++;
    //sleep(1);

    /*cursor = 0;
    mc_prop_ato_t pato;
    xbt_dynar_foreach(pair->propositions, cursor, pato){
      XBT_DEBUG("%s : %d", pato->id, pato->value);
      }*/

    /* Test acceptance pair and acceptance cycle*/

    if(pair->automaton_state->type == 1){
      if(search_cycle == 0){
	set_pair_prop_reached(pair);
	XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", pair->graph_state, pair->automaton_state, pair->automaton_state->id);
      }else{
	if(reached_prop(pair) == 1){
	  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	  XBT_INFO("|             ACCEPTANCE CYCLE            |");
	  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	  XBT_INFO("Counter-example that violates formula :");
	  MC_show_stack_liveness_stateful(mc_stack_liveness_stateful);
	  MC_dump_stack_liveness_stateful(mc_stack_liveness_stateful);
	  MC_print_statistics_pairs(mc_stats_pair);
	  exit(0);
	}
      }
    }

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack_liveness_stateful) < MAX_DEPTH &&
        (req = MC_state_get_request(pair->graph_state, &value))) {

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
        xbt_free(req_str);
      }

      MC_state_set_executed_request(pair->graph_state, req, value);
      //mc_stats_pairs->executed_transitions++;

      /* Answer the request */
      SIMIX_request_pre(req, value); /* After this call req is no longer usefull */

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_RAW_MEM;

      pair->interleave++;
      //XBT_DEBUG("Process interleaved in pair %p : %u", pair, MC_state_interleave_size(pair->graph_state));
      //xbt_dynar_pop(enabled_processes, NULL);

      next_graph_state = MC_state_new();

      next_system_state = xbt_new0(s_mc_snapshot_t, 1);
      MC_take_snapshot(next_system_state);
      MC_UNSET_RAW_MEM;

      cursor = 0;
      xbt_dynar_foreach(pair->automaton_state->out, cursor, transition_succ){
	res = MC_automaton_evaluate_label(a, transition_succ->label);	
	if((res == 1) || (res == 2)){ // enabled transition in automaton
	  next_automaton_state = transition_succ->dst;
	  break;
	}
      }

      if(next_automaton_state == NULL){
	next_automaton_state = pair->automaton_state;
      }

      MC_SET_RAW_MEM;
      
      next_pair = new_pair_prop(next_system_state, next_graph_state, next_automaton_state);
      cursor = 0;
      xbt_propositional_symbol_t ps;
      xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
	int (*f)() = ps->function;
	int val = (*f)();
	mc_prop_ato_t pa = new_proposition(ps->pred, val);
	xbt_dynar_push(next_pair->propositions, &pa);
	//XBT_DEBUG("%s : %d", pa->id, pa->value); 
      } 
      xbt_fifo_unshift(mc_stack_liveness_stateful, next_pair);
      
      cursor = 0;
      if((invisible(pair, next_pair) == 0) && (pair->fully_expanded == 0)){
	set_fully_expanded(pair);
	if(xbt_dynar_length(enabled_processes) > 1){ // Si 1 seul process activé, déjà exécuté juste avant donc état complètement étudié
	  xbt_dynar_foreach(enabled_processes, cursor, process){
	    MC_state_interleave_process(pair->graph_state, process);
	  }
	}
	XBT_DEBUG("Pair %p fully expanded (%u interleave)", pair, MC_state_interleave_size(pair->graph_state));
      }

      MC_UNSET_RAW_MEM;

     
      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {
      XBT_DEBUG("There are no more processes to interleave.");

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_liveness_stateful);
      //MC_state_delete(state);
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
      while ((pair = xbt_fifo_shift(mc_stack_liveness_stateful)) != NULL) {
        req = MC_state_get_internal_request(pair->graph_state);
        xbt_fifo_foreach(mc_stack_liveness_stateful, item, prev_pair, mc_pair_prop_t) {
          if(MC_request_depend(req, MC_state_get_internal_request(prev_pair->graph_state))){
            if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
              XBT_DEBUG("Dependent Transitions:");
              prev_req = MC_state_get_executed_request(prev_pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (pair=%p)", req_str, prev_pair);
              xbt_free(req_str);
              prev_req = MC_state_get_executed_request(pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (pair=%p)", req_str, pair);
              xbt_free(req_str);              
            }

	    if(prev_pair->fully_expanded == 0){
	      if(!MC_state_process_is_done(prev_pair->graph_state, req->issuer)){
		MC_state_interleave_process(prev_pair->graph_state, req->issuer);
		XBT_DEBUG("Process %p (%lu) interleaved in pair %p", req->issuer, req->issuer->pid, prev_pair);
	      }else{
		XBT_DEBUG("Process %p (%lu) is in done set", req->issuer, req->issuer->pid);
	      }
	    }
      
            break;
          }
        }

	
        if (MC_state_interleave_size(pair->graph_state) > 0) {
          /* We found a back-tracking point, let's loop */
	  MC_restore_snapshot(pair->system_state);
          xbt_fifo_unshift(mc_stack_liveness_stateful, pair);
          XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_stack_liveness_stateful));
          MC_UNSET_RAW_MEM;
          break;
        } else {
          //MC_state_delete(state);
        }
      }
      MC_UNSET_RAW_MEM;
    }
  }
  MC_UNSET_RAW_MEM;
  return;
}

/************ DPOR 3 (invisible and independant transitions with coloration of pairs) ************/

int expanded;
xbt_dynar_t reached_pairs_prop_col;
xbt_dynar_t visited_pairs_prop_col;



mc_pair_prop_col_t new_pair_prop_col(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  
  mc_pair_prop_col_t pair = NULL;
  pair = xbt_new0(s_mc_pair_prop_col_t, 1);
  pair->system_state = sn;
  pair->automaton_state = st;
  pair->graph_state = sg;
  pair->propositions = xbt_dynar_new(sizeof(mc_prop_ato_t), NULL);
  pair->fully_expanded = 0;
  pair->interleave = 0;
  pair->color=ORANGE;
  mc_stats_pair->expanded_pairs++;
  //XBT_DEBUG("New pair %p : automaton=%p", pair, st);
  return pair;
}

void set_expanded(mc_pair_prop_col_t pair){
  pair->expanded = expanded;
}

int reached_prop_col(mc_pair_prop_col_t pair){

  char* hash_reached = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&pair, hash);
  xbt_dynar_foreach(reached_pairs_prop_col, c, hash_reached){
    if(strcmp(hash, hash_reached) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_prop_col_reached(mc_pair_prop_col_t pair){

  if(reached_prop_col(pair) == 0){
    MC_SET_RAW_MEM;
    char *hash = malloc(sizeof(char)*160) ;
    xbt_sha((char *)&pair, hash);
    xbt_dynar_push(reached_pairs_prop_col, &hash); 
    MC_UNSET_RAW_MEM;
  }

}

int invisible_col(mc_pair_prop_col_t p, mc_pair_prop_col_t np){
  mc_prop_ato_t prop1;
  mc_prop_ato_t prop2;
  int i;
  for(i=0;i<xbt_dynar_length(p->propositions); i++){
    prop1 = xbt_dynar_get_as(p->propositions, i, mc_prop_ato_t);
    prop2 = xbt_dynar_get_as(np->propositions, i, mc_prop_ato_t);
    //XBT_DEBUG("test invisible : prop 1 = %s : %d / prop 2 = %s : %d", prop1->id, prop1->value, prop2->id, prop2->value);
    if(prop1->value != prop2->value)
	  return 0;
  }
  return 1; 

}

void set_fully_expanded_col(mc_pair_prop_col_t pair){
  pair->fully_expanded = 1;
  pair->color = GREEN;
}

void set_pair_prop_col_visited(mc_pair_prop_col_t pair, int sc){

  MC_SET_RAW_MEM;
  mc_visited_pair_prop_col_t p = NULL;
  p = xbt_new0(s_mc_visited_pair_prop_col_t, 1);
  p->pair = pair;
  p->search_cycle = sc;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  xbt_dynar_push(visited_pairs_prop_col, &hash); 
  MC_UNSET_RAW_MEM;	

}

int visited_pair_prop_col(mc_pair_prop_col_t pair, int sc){

  char* hash_visited = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  mc_visited_pair_prop_col_t p = NULL;
  p = xbt_new0(s_mc_visited_pair_prop_col_t, 1);
  p->pair = pair;
  p->search_cycle = sc;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  xbt_dynar_foreach(visited_pairs_prop_col, c, hash_visited){
    if(strcmp(hash, hash_visited) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;

}

void MC_dpor3_init(xbt_automaton_t a)
{
  mc_pair_prop_col_t initial_pair = NULL;
  mc_state_t initial_graph_state = NULL;
  mc_snapshot_t initial_system_state = NULL;
  xbt_state_t initial_automaton_state = NULL;
 

  MC_SET_RAW_MEM;
  initial_graph_state = MC_state_new();
  MC_UNSET_RAW_MEM;

  /* Wait for requests (schedules processes) */
  MC_wait_for_requests();

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t automaton_state = NULL;
  int res;
  xbt_transition_t transition_succ;

  xbt_dynar_foreach(a->states, cursor, automaton_state){
    if(automaton_state->type == -1){
      xbt_dynar_foreach(automaton_state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
  	if((res == 1) || (res == 2)){
	  initial_automaton_state = transition_succ->dst;
	  break;
	}
      }
    }

    if(initial_automaton_state != NULL)
      break;
  }

  if(initial_automaton_state == NULL){
    cursor = 0;
    xbt_dynar_foreach(a->states, cursor, automaton_state){
      if(automaton_state->type == -1){
	initial_automaton_state = automaton_state;
	break;
      }
    }
  }

  reached_pairs_prop_col = xbt_dynar_new(sizeof(char *), NULL); 
  visited_pairs_prop_col = xbt_dynar_new(sizeof(char *), NULL); 
  visible_transitions = xbt_dynar_new(sizeof(s_smx_req_t), NULL);
  enabled_processes = xbt_dynar_new(sizeof(smx_process_t), NULL);
  expanded = 0;

  MC_SET_RAW_MEM;
  initial_system_state = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_system_state);
  initial_pair = new_pair_prop_col(initial_system_state, initial_graph_state, initial_automaton_state);
  cursor = 0;
  xbt_propositional_symbol_t ps;
  xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
    int (*f)() = ps->function;
    int val = (*f)();
    mc_prop_ato_t pa = new_proposition(ps->pred, val);
    xbt_dynar_push(initial_pair->propositions, &pa);
  } 
  xbt_fifo_unshift(mc_stack_liveness_stateful, initial_pair);
  MC_UNSET_RAW_MEM;


  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Initial pair");

  MC_dpor3(a, 0);
    
}


void MC_dpor3(xbt_automaton_t a, int search_cycle)
{
  char *req_str;
  int value;
  smx_req_t req = NULL, prev_req = NULL;
  mc_state_t next_graph_state = NULL;
  mc_snapshot_t next_system_state = NULL;
  xbt_state_t next_automaton_state = NULL;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;
  mc_pair_prop_col_t pair = NULL, next_pair = NULL, prev_pair = NULL;
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  int d;


  while (xbt_fifo_size(mc_stack_liveness_stateful) > 0) {

    /* Get current pair */
    pair = (mc_pair_prop_col_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));

    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Exploration depth=%d (pair=%p) (%d interleave)",
	      xbt_fifo_size(mc_stack_liveness_stateful), pair, MC_state_interleave_size(pair->graph_state));

    if(MC_state_interleave_size(pair->graph_state) == 0){
    
      xbt_dynar_reset(enabled_processes);

      MC_SET_RAW_MEM;
    
      xbt_swag_foreach(process, simix_global->process_list){
	if(MC_process_is_enabled(process)){ 
	  xbt_dynar_push(enabled_processes, &process);
	  //XBT_DEBUG("Process : pid=%lu",process->pid);
	}
      }

      //XBT_DEBUG("Enabled processes before pop : %lu", xbt_dynar_length(enabled_processes));
    
      //XBT_DEBUG("Processes already interleaved : %d", pair->interleave);
    
      if(xbt_dynar_length(enabled_processes) > 0){
	for(d=0;d<pair->interleave;d++){
	  xbt_dynar_shift(enabled_processes, NULL);
	}
      }

      //XBT_DEBUG("Enabled processes after pop : %lu", xbt_dynar_length(enabled_processes));
    
      if(pair->fully_expanded == 0){
	if(xbt_dynar_length(enabled_processes) > 0){
	  MC_state_interleave_process(pair->graph_state, xbt_dynar_get_as(enabled_processes, 0, smx_process_t));
	  //XBT_DEBUG("Interleave process");
	}
      }
    
      MC_UNSET_RAW_MEM;

    }


    /* Update statistics */
    mc_stats_pair->visited_pairs++;
    //sleep(1);

    /* Test acceptance pair and acceptance cycle*/

    if(pair->automaton_state->type == 1){
      if(search_cycle == 0){
	set_pair_prop_col_reached(pair);
	XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", pair->graph_state, pair->automaton_state, pair->automaton_state->id);
      }else{
	if(reached_prop_col(pair) == 1){
	  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	  XBT_INFO("|             ACCEPTANCE CYCLE            |");
	  XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	  XBT_INFO("Counter-example that violates formula :");
	  MC_show_stack_liveness_stateful(mc_stack_liveness_stateful);
	  MC_dump_stack_liveness_stateful(mc_stack_liveness_stateful);
	  MC_print_statistics_pairs(mc_stats_pair);
	  exit(0);
	}
      }
    }

    /* If there are processes to interleave and the maximum depth has not been reached
       then perform one step of the exploration algorithm */
    if (xbt_fifo_size(mc_stack_liveness_stateful) < MAX_DEPTH &&
        (req = MC_state_get_request(pair->graph_state, &value))) {

      set_pair_prop_col_visited(pair, search_cycle);
      
      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Execute: %s", req_str);
        xbt_free(req_str);
      }

      MC_state_set_executed_request(pair->graph_state, req, value);

      /* Answer the request */
      SIMIX_request_pre(req, value); /* After this call req is no longer usefull */

      /* Wait for requests (schedules processes) */
      MC_wait_for_requests();

      /* Create the new expanded state */
      MC_SET_RAW_MEM;

      pair->interleave++;

      next_graph_state = MC_state_new();

      next_system_state = xbt_new0(s_mc_snapshot_t, 1);
      MC_take_snapshot(next_system_state);
      MC_UNSET_RAW_MEM;

      cursor = 0;
      xbt_dynar_foreach(pair->automaton_state->out, cursor, transition_succ){
	res = MC_automaton_evaluate_label(a, transition_succ->label);	
	if((res == 1) || (res == 2)){ // enabled transition in automaton
	  next_automaton_state = transition_succ->dst;
	  break;
	}
      }

      if(next_automaton_state == NULL)
	next_automaton_state = pair->automaton_state;

      MC_SET_RAW_MEM;
      
      next_pair = new_pair_prop_col(next_system_state, next_graph_state, next_automaton_state);
      cursor = 0;
      xbt_propositional_symbol_t ps;
      xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
	int (*f)() = ps->function;
	int val = (*f)();
	mc_prop_ato_t pa = new_proposition(ps->pred, val);
	xbt_dynar_push(next_pair->propositions, &pa);
      } 
      xbt_fifo_unshift(mc_stack_liveness_stateful, next_pair);
      
      cursor = 0;
      if((invisible_col(pair, next_pair) == 0) && (pair->fully_expanded == 0)){
	set_fully_expanded_col(pair);
	if(xbt_dynar_length(enabled_processes) > 1){ // Si 1 seul process activé, déjà exécuté juste avant donc état complètement étudié
	  xbt_dynar_foreach(enabled_processes, cursor, process){
	    MC_state_interleave_process(pair->graph_state, process);
	  }
	}
	XBT_DEBUG("Pair %p fully expanded (%u interleave)", pair, MC_state_interleave_size(pair->graph_state));
      }

      MC_UNSET_RAW_MEM;

     
      /* Let's loop again */

      /* The interleave set is empty or the maximum depth is reached, let's back-track */
    } else {
      XBT_DEBUG("There are no more processes to interleave.");

      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack_liveness_stateful);
      //MC_state_delete(state);
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
      while ((pair = xbt_fifo_shift(mc_stack_liveness_stateful)) != NULL) {
        req = MC_state_get_internal_request(pair->graph_state);
        xbt_fifo_foreach(mc_stack_liveness_stateful, item, prev_pair, mc_pair_prop_col_t) {
          if(MC_request_depend(req, MC_state_get_internal_request(prev_pair->graph_state))){
            if(XBT_LOG_ISENABLED(mc_dpor, xbt_log_priority_debug)){
              XBT_DEBUG("Dependent Transitions:");
              prev_req = MC_state_get_executed_request(prev_pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (pair=%p)", req_str, prev_pair);
              xbt_free(req_str);
              prev_req = MC_state_get_executed_request(pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (pair=%p)", req_str, pair);
              xbt_free(req_str);              
            }

	    if(prev_pair->fully_expanded == 0){
	      if(!MC_state_process_is_done(prev_pair->graph_state, req->issuer)){
		MC_state_interleave_process(prev_pair->graph_state, req->issuer);
		XBT_DEBUG("Process %p (%lu) interleaved in pair %p", req->issuer, req->issuer->pid, prev_pair);
	      }else{
		XBT_DEBUG("Process %p (%lu) is in done set", req->issuer, req->issuer->pid);
	      }
	    }
      
            break;
          }
        }

	
        if (MC_state_interleave_size(pair->graph_state) > 0) {
          /* We found a back-tracking point, let's loop */
	  MC_restore_snapshot(pair->system_state);
          xbt_fifo_unshift(mc_stack_liveness_stateful, pair);
          XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_stack_liveness_stateful));
          MC_UNSET_RAW_MEM;
          break;
        } else {
          //MC_state_delete(state);
        }
      }
      MC_UNSET_RAW_MEM;
    }
  }
  MC_UNSET_RAW_MEM;
  return;
}
