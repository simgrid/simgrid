#include "private.h"
#include "unistd.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

xbt_dynar_t initial_pairs = NULL;
xbt_dynar_t reached_pairs;
extern mc_snapshot_t initial_snapshot;

mc_pair_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->system_state = sn;
  p->automaton_state = st;
  p->graph_state = sg;
  mc_stats_pair->expanded_pairs++;
  return p;
    
}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){
  
  if(s1->num_reg != s2->num_reg)
    return 1;

  int i, errors=0;

  for(i=0 ; i< s1->num_reg ; i++){
    
    if(s1->regions[i]->size != s2->regions[i]->size)
      return 1;
    
    if(s1->regions[i]->start_addr != s2->regions[i]->start_addr)
      return 1;
    
    if(s1->regions[i]->type != s2->regions[i]->type)
      return 1;

    if(s1->regions[i]->type == 0){ 
      if(mmalloc_compare_heap(s1->regions[i]->start_addr, s2->regions[i]->start_addr)){
	XBT_DEBUG("Different heap (mmalloc_compare)");
	//sleep(1);
	errors++; 
      }
    }else{
      if(memcmp(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
    	XBT_DEBUG("Different memcmp for data in libsimgrid (%d)", i);
	//sleep(2);
    	errors++;
      }
    }
    
    
  }

  return (errors>0);

}

int reached(xbt_automaton_t a, mc_snapshot_t s){

  if(xbt_dynar_is_empty(reached_pairs)){
    return 0;
  }else{
    MC_SET_RAW_MEM;
  
    mc_pair_reached_t pair = NULL;
    pair = xbt_new0(s_mc_pair_reached_t, 1);
    pair->automaton_state = a->current_state;
    pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
    pair->system_state = s;
    
    /* Get values of propositional symbols */
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
      int (*f)() = ps->function;
      int res = (*f)();
      xbt_dynar_push(pair->prop_ato, &res);
    }
    
    cursor = 0;
    mc_pair_reached_t pair_test;
    int (*compare_dynar)(const void*; const void*) = propositional_symbols_compare;
    
    xbt_dynar_foreach(reached_pairs, cursor, pair_test){
      if(automaton_state_compare(pair_test->automaton_state, pair->automaton_state) == 0){
	if(xbt_dynar_compare(pair_test->prop_ato, pair->prop_ato, compare_dynar) == 0){
	  if(snapshot_compare(pair_test->system_state, pair->system_state) == 0){
	    MC_UNSET_RAW_MEM;
	    return 1;
	  }
	}
      }
    }

    MC_UNSET_RAW_MEM;
    return 0;
    
  }
}

int set_pair_reached(xbt_automaton_t a, mc_snapshot_t s){

  if(reached(a, s) == 0){

    MC_SET_RAW_MEM;

    mc_pair_reached_t pair = NULL;
    pair = xbt_new0(s_mc_pair_reached_t, 1);
    pair->automaton_state = a->current_state;
    pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
    pair->system_state = s;
    
    /* Get values of propositional symbols */
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    xbt_dynar_foreach(a->propositional_symbols, cursor, ps){
      int (*f)() = ps->function;
      int res = (*f)();
      xbt_dynar_push(pair->prop_ato, &res);
    }
     
    xbt_dynar_push(reached_pairs, &pair); 
   
    MC_UNSET_RAW_MEM;

    return 1;

  }

  return 0;
}

void MC_pair_delete(mc_pair_t pair){
  xbt_free(pair->graph_state->proc_status);
  xbt_free(pair->graph_state);
  //xbt_free(pair->automaton_state); -> FIXME : à implémenter
  xbt_free(pair);
}


int MC_automaton_evaluate_label(xbt_automaton_t a, xbt_exp_label_t l){
  
  switch(l->type){
  case 0 : {
    int left_res = MC_automaton_evaluate_label(a, l->u.or_and.left_exp);
    int right_res = MC_automaton_evaluate_label(a, l->u.or_and.right_exp);
    return (left_res || right_res);
    break;
  }
  case 1 : {
    int left_res = MC_automaton_evaluate_label(a, l->u.or_and.left_exp);
    int right_res = MC_automaton_evaluate_label(a, l->u.or_and.right_exp);
    return (left_res && right_res);
    break;
  }
  case 2 : {
    int res = MC_automaton_evaluate_label(a, l->u.exp_not);
    return (!res);
    break;
  }
  case 3 : { 
    unsigned int cursor = 0;
    xbt_propositional_symbol_t p = NULL;
    xbt_dynar_foreach(a->propositional_symbols, cursor, p){
      if(strcmp(p->pred, l->u.predicat) == 0){
	int (*f)() = p->function;
	return (*f)();
      }
    }
    return -1;
    break;
  }
  case 4 : {
    return 2;
    break;
  }
  default : 
    return -1;
  }
}





/********************* Double-DFS stateless *******************/

void MC_pair_stateless_delete(mc_pair_stateless_t pair){
  xbt_free(pair->graph_state->proc_status);
  xbt_free(pair->graph_state);
  //xbt_free(pair->automaton_state); -> FIXME : à implémenter
  xbt_free(pair);
}

mc_pair_stateless_t new_pair_stateless(mc_state_t sg, xbt_state_t st){
  mc_pair_stateless_t p = NULL;
  p = xbt_new0(s_mc_pair_stateless_t, 1);
  p->automaton_state = st;
  p->graph_state = sg;
  mc_stats_pair->expanded_pairs++;
  return p;
}



void MC_ddfs_stateless_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS stateless init");
  XBT_DEBUG("**************************************************");
 
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

  reached_pairs = xbt_dynar_new(sizeof(mc_pair_reached_t), NULL); 

  initial_snapshot = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_snapshot);

  MC_UNSET_RAW_MEM; 

  unsigned int cursor = 0;
  xbt_state_t state;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      
      MC_SET_RAW_MEM;
      mc_initial_pair = new_pair_stateless(initial_graph_state, state);
      xbt_fifo_unshift(mc_stack_liveness_stateless, mc_initial_pair);
      MC_UNSET_RAW_MEM;
      
      if(cursor == 0){
	MC_ddfs_stateless(a, 0, 0);
      }else{
	MC_restore_snapshot(initial_snapshot);
	MC_UNSET_RAW_MEM;
	MC_ddfs_stateless(a, 0, 0);
      }
    }else{
      if(state->type == 2){
      
	MC_SET_RAW_MEM;
	mc_initial_pair = new_pair_stateless(initial_graph_state, state);
	xbt_fifo_unshift(mc_stack_liveness_stateless, mc_initial_pair);
	MC_UNSET_RAW_MEM;
	
	if(cursor == 0){
	  MC_ddfs_stateless(a, 1, 0);
	}else{
	  MC_restore_snapshot(initial_snapshot);
	  MC_UNSET_RAW_MEM;
	  MC_ddfs_stateless(a, 1, 0);
	}
      }
    }
  } 

}


void MC_ddfs_stateless(xbt_automaton_t a, int search_cycle, int replay){

  smx_process_t process;
  mc_pair_stateless_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness_stateless) == 0)
    return;

  if(replay == 1){
    MC_replay_liveness(mc_stack_liveness_stateless);
    current_pair = (mc_pair_stateless_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateless));
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(current_pair->graph_state, process);
      }
    }
  }

  /* Get current pair */
  current_pair = (mc_pair_stateless_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateless));

  /* Update current state in buchi automaton */
  a->current_state = current_pair->automaton_state;
 
  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness_stateless), search_cycle);
  XBT_DEBUG("Pair : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));

  
  mc_stats_pair->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  xbt_transition_t transition_succ;
  unsigned int cursor = 0;
  int res;

  mc_pair_stateless_t next_pair = NULL;
  mc_pair_stateless_t pair_succ;
  mc_snapshot_t next_snapshot = NULL;
  mc_snapshot_t current_snapshot = NULL;
  
  xbt_dynar_t successors = NULL;
  
  MC_SET_RAW_MEM;
  successors = xbt_dynar_new(sizeof(mc_pair_stateless_t), NULL);
  MC_UNSET_RAW_MEM;

  while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){
   
    MC_SET_RAW_MEM;
    current_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(current_snapshot);
    MC_UNSET_RAW_MEM;

    /* Debug information */
    if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
      req_str = MC_request_to_string(req, value);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);
    }

    //sleep(1);

    MC_state_set_executed_request(current_pair->graph_state, req, value);   
    
    /* Answer the request */
    SIMIX_request_pre(req, value);

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

    next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(next_snapshot);
    
    xbt_dynar_reset(successors);

    if(snapshot_compare(current_snapshot,next_snapshot)){
      XBT_DEBUG("Different snapshot");
      //sleep(2);
      
    }

    MC_UNSET_RAW_MEM;
    

    cursor= 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);

      if(res == 1){ // enabled transition in automaton
	MC_SET_RAW_MEM;
	next_pair = new_pair_stateless(next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
	MC_UNSET_RAW_MEM;
      }

    }

    cursor = 0;
   
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      
      res = MC_automaton_evaluate_label(a, transition_succ->label);
	
      if(res == 2){ // true transition in automaton
	MC_SET_RAW_MEM;
	next_pair = new_pair_stateless(next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
	MC_UNSET_RAW_MEM;
      }

    }

   
    if(xbt_dynar_length(successors) == 0){
      MC_SET_RAW_MEM;
      next_pair = new_pair_stateless(next_graph_state, current_pair->automaton_state);
      xbt_dynar_push(successors, &next_pair);
      MC_UNSET_RAW_MEM;
    }

    cursor = 0; 

    xbt_dynar_foreach(successors, cursor, pair_succ){
     
      if((search_cycle == 1) && (reached(a, next_snapshot) == 1)){
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("|             ACCEPTANCE CYCLE            |");
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("Counter-example that violates formula :");
	MC_show_stack_liveness_stateless(mc_stack_liveness_stateless);
	MC_dump_stack_liveness_stateless(mc_stack_liveness_stateless);
	MC_print_statistics_pairs(mc_stats_pair);
	exit(0);
      }
	
      MC_SET_RAW_MEM;
      xbt_fifo_unshift(mc_stack_liveness_stateless, pair_succ);
      MC_UNSET_RAW_MEM;

      MC_ddfs_stateless(a, search_cycle, 0);

     
      if((search_cycle == 0) && ((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

	XBT_DEBUG("Acceptance pair %p : graph=%p, automaton=%p(%s)", pair_succ, pair_succ->graph_state, pair_succ->automaton_state, pair_succ->automaton_state->id);
	int res = set_pair_reached(a, next_snapshot);

	MC_SET_RAW_MEM;
	xbt_fifo_unshift(mc_stack_liveness_stateless, pair_succ);
	MC_UNSET_RAW_MEM;
      
	MC_ddfs_stateless(a, 1, 1);
	
	if(res){
	  MC_SET_RAW_MEM;
	  xbt_dynar_pop(reached_pairs, NULL);
	  MC_UNSET_RAW_MEM;
	}
      }
    }

    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      XBT_DEBUG("Backtracking to depth %u", xbt_fifo_size(mc_stack_liveness_stateless));
      MC_replay_liveness(mc_stack_liveness_stateless);
    }    
   
  }
  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_stack_liveness_stateless);
  XBT_DEBUG("Pair (graph=%p, automaton =%p, search_cycle = %u) shifted in stack", current_pair->graph_state, current_pair->automaton_state, search_cycle);
  MC_UNSET_RAW_MEM;

}

/********************* Double-DFS stateful without visited state *******************/


void MC_ddfs_stateful_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS stateful without visited state init");
  XBT_DEBUG("**************************************************");
 
  mc_pair_t mc_initial_pair;
  mc_state_t initial_graph_state;
  smx_process_t process; 
  mc_snapshot_t init_snapshot;
 
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  init_snapshot = xbt_new0(s_mc_snapshot_t, 1);

  initial_graph_state = MC_state_pair_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  reached_pairs = xbt_dynar_new(sizeof(mc_pair_reached_t), NULL); 

  MC_take_snapshot(init_snapshot);

  MC_UNSET_RAW_MEM; 

  unsigned int cursor = 0;
  xbt_state_t state = NULL;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
    
      MC_SET_RAW_MEM;
      mc_initial_pair = new_pair(init_snapshot, initial_graph_state, state);
      xbt_fifo_unshift(mc_stack_liveness_stateful, mc_initial_pair);
      MC_UNSET_RAW_MEM;

      if(cursor == 0){
	MC_ddfs_stateful(a, 0, 0);
      }else{
	MC_restore_snapshot(init_snapshot);
	MC_UNSET_RAW_MEM;
	MC_ddfs_stateful(a, 0, 0);
      }
    }else{
       if(state->type == 2){
    
	 MC_SET_RAW_MEM;
	 mc_initial_pair = new_pair(init_snapshot, initial_graph_state, state);
	 xbt_fifo_unshift(mc_stack_liveness_stateful, mc_initial_pair);
	 MC_UNSET_RAW_MEM;
	 
	 if(cursor == 0){
	   MC_ddfs_stateful(a, 1, 0);
	 }else{
	   MC_restore_snapshot(init_snapshot);
	   MC_UNSET_RAW_MEM;
	   MC_ddfs_stateful(a, 1, 0);
	 }
       }
    }
  } 
}


void MC_ddfs_stateful(xbt_automaton_t a, int search_cycle, int restore){

  smx_process_t process = NULL;
  mc_pair_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness_stateful) == 0)
    return;

  if(restore == 1){
    current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));
    MC_restore_snapshot(current_pair->system_state);
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(current_pair->graph_state, process);
      }
    }
    MC_UNSET_RAW_MEM;
  }

  /* Get current state */
  current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));


  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness_stateful), search_cycle);
  XBT_DEBUG("Pair : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));

  a->current_state = current_pair->automaton_state;

  mc_stats_pair->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  mc_pair_t pair_succ;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;

  xbt_dynar_t successors = NULL;

  mc_pair_t next_pair = NULL;
  mc_snapshot_t next_snapshot = NULL;
  mc_snapshot_t current_snapshot = NULL;

  //sleep(1);
  MC_SET_RAW_MEM;
  successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);
  MC_UNSET_RAW_MEM;
  
  while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){

    MC_SET_RAW_MEM;
    current_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(current_snapshot);
    MC_UNSET_RAW_MEM;
   
    
    /* Debug information */
    if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
      req_str = MC_request_to_string(req, value);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);
    }

    MC_state_set_executed_request(current_pair->graph_state, req, value);

    /* Answer the request */
    SIMIX_request_pre(req, value);

    /* Wait for requests (schedules processes) */
    MC_wait_for_requests();


    /* Create the new expanded graph_state */
    MC_SET_RAW_MEM;

    next_graph_state = MC_state_pair_new();
    
    /* Get enabled process and insert it in the interleave set of the next graph_state */
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(next_graph_state, process);
      }
    }

    next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(next_snapshot);

    xbt_dynar_reset(successors);

    MC_UNSET_RAW_MEM;

    
    cursor = 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);
	
      if(res == 1){ // enabled transition in automaton
	MC_SET_RAW_MEM;
	next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
	MC_UNSET_RAW_MEM;
      }

    }

    cursor = 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);
	
      if(res == 2){ // transition always enabled in automaton
	MC_SET_RAW_MEM;
	next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
	MC_UNSET_RAW_MEM;
      }

     
    }

   
    if(xbt_dynar_length(successors) == 0){

      MC_SET_RAW_MEM;
      next_pair = new_pair(next_snapshot, next_graph_state, current_pair->automaton_state);
      xbt_dynar_push(successors, &next_pair);
      MC_UNSET_RAW_MEM;
	
    }

    //XBT_DEBUG("Successors in automaton %lu", xbt_dynar_length(successors));

    cursor = 0;
    xbt_dynar_foreach(successors, cursor, pair_succ){

      //XBT_DEBUG("Search visited pair : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);

      if((search_cycle == 1) && (reached(a, next_snapshot) == 1)){
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("|             ACCEPTANCE CYCLE            |");
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("Counter-example that violates formula :");
	MC_show_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_dump_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_print_statistics_pairs(mc_stats_pair);
	exit(0);
      }
	
      //mc_stats_pair->executed_transitions++;
 
      MC_SET_RAW_MEM;
      xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);
      MC_UNSET_RAW_MEM;

      MC_ddfs_stateful(a, search_cycle, 0);
    
    
      if((search_cycle == 0) && ((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

	int res = set_pair_reached(a, next_snapshot);
	XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", pair_succ->graph_state, pair_succ->automaton_state, pair_succ->automaton_state->id);
	
	MC_SET_RAW_MEM;
	xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);
	MC_UNSET_RAW_MEM;
	
	MC_ddfs_stateful(a, 1, 1);

	if(res){
	  MC_SET_RAW_MEM;
	  xbt_dynar_pop(reached_pairs, NULL);
	  MC_UNSET_RAW_MEM;
	}
      }

    }

    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      XBT_DEBUG("Backtracking to depth %u", xbt_fifo_size(mc_stack_liveness_stateful));
      MC_restore_snapshot(current_snapshot);
      MC_UNSET_RAW_MEM;
    }

  }

  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_stack_liveness_stateful);
  XBT_DEBUG("Pair (graph=%p, automaton =%p) shifted in stack", current_pair->graph_state, current_pair->automaton_state);
  MC_UNSET_RAW_MEM;

}
