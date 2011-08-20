#include "private.h"
#include "unistd.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

xbt_dynar_t initial_pairs = NULL;
int max_pair = 0;
xbt_dynar_t reached_pairs;

mc_pair_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pair_t p = NULL;
  p = xbt_new0(s_mc_pair_t, 1);
  p->system_state = sn;
  p->automaton_state = st;
  p->graph_state = sg;
  p->num = max_pair;
  max_pair++;
  mc_stats_pair->expanded_pairs++;
  return p;
    
}


int reached(mc_pair_t pair){

  char* hash_reached = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&pair, hash);
  xbt_dynar_foreach(reached_pairs, c, hash_reached){
    if(strcmp(hash, hash_reached) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_reached(mc_pair_t pair){

  if(reached(pair) == 0){
    MC_SET_RAW_MEM;
    char *hash = malloc(sizeof(char)*160) ;
    xbt_sha((char *)&pair, hash);
    xbt_dynar_push(reached_pairs, &hash); 
    MC_UNSET_RAW_MEM;
  }

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



/******************************* DFS with visited state *******************************/

xbt_dynar_t visited_pairs;

void set_pair_visited(mc_pair_t pair, int sc){

  MC_SET_RAW_MEM;
  mc_visited_pair_t p = NULL;
  p = xbt_new0(s_mc_visited_pair_t, 1);
  p->pair = pair;
  p->search_cycle = sc;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  xbt_dynar_push(visited_pairs, &hash); 
  MC_UNSET_RAW_MEM;	

}

int visited(mc_pair_t pair, int sc){

  char* hash_visited = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  mc_visited_pair_t p = NULL;
  p = xbt_new0(s_mc_visited_pair_t, 1);
  p->pair = pair;
  p->search_cycle = sc;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  xbt_dynar_foreach(visited_pairs, c, hash_visited){
    if(strcmp(hash, hash_visited) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;

}


void MC_vddfs_stateful_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS stateful with visited state init");
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

  visited_pairs = xbt_dynar_new(sizeof(char *), NULL); 
  reached_pairs = xbt_dynar_new(sizeof(char *), NULL); 

  MC_take_snapshot(init_snapshot);

  MC_UNSET_RAW_MEM; 

  /* regarder dans l'automate toutes les transitions activables grâce à l'état initial du système 
    -> donnera les états initiaux de la propriété consistants avec l'état initial du système */

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);
  mc_pair_t pair_succ;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
  	if((res == 1) || (res == 2)){
	 
  	  MC_SET_RAW_MEM;

  	  mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
  	  xbt_dynar_push(successors, &mc_initial_pair);

  	  MC_UNSET_RAW_MEM;

	}
      }
    }
  }

  cursor = 0;

  if(xbt_dynar_length(successors) == 0){
    xbt_dynar_foreach(a->states, cursor, state){
      if(state->type == -1){
	MC_SET_RAW_MEM;

	mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &mc_initial_pair);
	
	MC_UNSET_RAW_MEM;
      }
    }
  }

  cursor = 0;
  xbt_dynar_foreach(successors, cursor, pair_succ){
    MC_SET_RAW_MEM;

    xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);
    set_pair_visited(pair_succ, 0);

    MC_UNSET_RAW_MEM;

    if(cursor == 0){
      MC_vddfs_stateful(a, 0, 0);
    }else{
      MC_vddfs_stateful(a, 0, 1);
    }
  }  
}


void MC_vddfs_stateful(xbt_automaton_t a, int search_cycle, int restore){

  smx_process_t process = NULL;


  if(xbt_fifo_size(mc_stack_liveness_stateful) == 0)
    return;

  if(restore == 1){
    MC_restore_snapshot(((mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful)))->system_state);
    MC_UNSET_RAW_MEM;
  }


  /* Get current state */
  mc_pair_t current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));

  if(restore==1){
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(current_pair->graph_state, process);
      }
    }
  }

  XBT_DEBUG("************************************************** ( search_cycle = %d )", search_cycle);
  XBT_DEBUG("Pair : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));
  

  mc_stats_pair->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  mc_pair_t pair_succ;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;

  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);

  mc_pair_t next_pair;
  mc_snapshot_t next_snapshot;
  mc_snapshot_t current_snapshot;
  
  
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
      
    MC_UNSET_RAW_MEM;

    xbt_dynar_reset(successors);
    
    cursor = 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);

      MC_SET_RAW_MEM;
	
      if((res == 1) || (res == 2)){ // enabled transition in automaton
	next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
      }

      MC_UNSET_RAW_MEM;
    }

   
    if(xbt_dynar_length(successors) == 0){

      MC_SET_RAW_MEM;
      next_pair = new_pair(next_snapshot, next_graph_state, current_pair->automaton_state);
      xbt_dynar_push(successors, &next_pair);
      MC_UNSET_RAW_MEM;
	
    }


    cursor = 0;
    xbt_dynar_foreach(successors, cursor, pair_succ){

      //XBT_DEBUG("Search visited pair : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);

      if((search_cycle == 1) && (reached(pair_succ) == 1)){
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("|             ACCEPTANCE CYCLE            |");
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("Counter-example that violates formula :");
	MC_show_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_dump_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_print_statistics_pairs(mc_stats_pair);
	exit(0);
      }
	
      if(visited(pair_succ, search_cycle) == 0){

	mc_stats_pair->executed_transitions++;
 
	MC_SET_RAW_MEM;
	xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);
	set_pair_visited(pair_succ, search_cycle);
	MC_UNSET_RAW_MEM;

	MC_vddfs_stateful(a, search_cycle, 0);

	if((search_cycle == 0) && (current_pair->automaton_state->type == 1)){

	  MC_restore_snapshot(current_pair->system_state);
	  MC_UNSET_RAW_MEM;
 
	  xbt_swag_foreach(process, simix_global->process_list){
	    if(MC_process_is_enabled(process)){
	      MC_state_interleave_process(current_pair->graph_state, process);
	    }
	  }
	    
	  set_pair_reached(current_pair);
	  XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id);
	  MC_vddfs_stateful(a, 1, 1);

	}
      }else{

	XBT_DEBUG("Pair already visited !");

      }
    }
    
    
    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      MC_restore_snapshot(current_snapshot);
      MC_UNSET_RAW_MEM;
    }
  }

  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_stack_liveness_stateful);
  XBT_DEBUG("Pair (graph=%p, automaton =%p) shifted in snapshot_stack", current_pair->graph_state, current_pair->automaton_state);
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

  reached_pairs = xbt_dynar_new(sizeof(char *), NULL); 

  MC_take_snapshot(init_snapshot);

  MC_UNSET_RAW_MEM; 

  /* regarder dans l'automate toutes les transitions activables grâce à l'état initial du système 
    -> donnera les états initiaux de la propriété consistants avec l'état initial du système */

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);
  mc_pair_t pair_succ;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
  	if((res == 1) || (res == 2)){
	 
  	  MC_SET_RAW_MEM;

  	  mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
  	  xbt_dynar_push(successors, &mc_initial_pair);

  	  MC_UNSET_RAW_MEM;

	}
      }
    }
  }

  cursor = 0;

  if(xbt_dynar_length(successors) == 0){
    xbt_dynar_foreach(a->states, cursor, state){
      if(state->type == -1){
	MC_SET_RAW_MEM;

	mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &mc_initial_pair);
	
	MC_UNSET_RAW_MEM;
      }
    }
  }

  cursor = 0;
  xbt_dynar_foreach(successors, cursor, pair_succ){
    MC_SET_RAW_MEM;

    xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);

    MC_UNSET_RAW_MEM;

    if(cursor == 0){
      MC_ddfs_stateful(a, 0, 0);
    }else{
      MC_ddfs_stateful(a, 0, 1);
    }
  }  
}


void MC_ddfs_stateful(xbt_automaton_t a, int search_cycle, int restore){

  smx_process_t process = NULL;


  if(xbt_fifo_size(mc_stack_liveness_stateful) == 0)
    return;

  if(restore == 1){
    MC_restore_snapshot(((mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful)))->system_state);
    MC_UNSET_RAW_MEM;
  }


  /* Get current state */
  mc_pair_t current_pair = (mc_pair_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness_stateful));

  if(restore==1){
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(current_pair->graph_state, process);
      }
    }
  }

  XBT_DEBUG("************************************************** ( search_cycle = %d )", search_cycle);
  XBT_DEBUG("Pair : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));
  

  mc_stats_pair->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  mc_pair_t pair_succ;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;

  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pair_t), NULL);

  mc_pair_t next_pair;
  mc_snapshot_t next_snapshot;
  mc_snapshot_t current_snapshot;
  
  
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
      
    MC_UNSET_RAW_MEM;

    xbt_dynar_reset(successors);
    
    cursor = 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);

      MC_SET_RAW_MEM;
	
      if((res == 1) || (res == 2)){ // enabled transition in automaton
	next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &next_pair);
      }

      MC_UNSET_RAW_MEM;
    }

   
    if(xbt_dynar_length(successors) == 0){

      MC_SET_RAW_MEM;
      next_pair = new_pair(next_snapshot, next_graph_state, current_pair->automaton_state);
      xbt_dynar_push(successors, &next_pair);
      MC_UNSET_RAW_MEM;
	
    }


    cursor = 0;
    xbt_dynar_foreach(successors, cursor, pair_succ){

      //XBT_DEBUG("Search visited pair : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);

      if((search_cycle == 1) && (reached(pair_succ) == 1)){
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("|             ACCEPTANCE CYCLE            |");
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("Counter-example that violates formula :");
	MC_show_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_dump_stack_liveness_stateful(mc_stack_liveness_stateful);
	MC_print_statistics_pairs(mc_stats_pair);
	exit(0);
      }
	
      mc_stats_pair->executed_transitions++;
 
      MC_SET_RAW_MEM;
      xbt_fifo_unshift(mc_stack_liveness_stateful, pair_succ);
      MC_UNSET_RAW_MEM;

      MC_ddfs_stateful(a, search_cycle, 0);

      if((search_cycle == 0) && (current_pair->automaton_state->type == 1)){

	MC_restore_snapshot(current_pair->system_state);
	MC_UNSET_RAW_MEM;
 
	xbt_swag_foreach(process, simix_global->process_list){
	  if(MC_process_is_enabled(process)){
	    MC_state_interleave_process(current_pair->graph_state, process);
	  }
	}
	    
	set_pair_reached(current_pair);
	XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id);
	MC_ddfs_stateful(a, 1, 1);

      }
    }
    
    
    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      MC_restore_snapshot(current_snapshot);
      MC_UNSET_RAW_MEM;
    }
  }

  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_stack_liveness_stateful);
  XBT_DEBUG("Pair (graph=%p, automaton =%p) shifted in snapshot_stack", current_pair->graph_state, current_pair->automaton_state);
  MC_UNSET_RAW_MEM;

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


int reached_stateless(mc_pair_stateless_t pair){

  char* hash_reached = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&pair, hash);
  xbt_dynar_foreach(reached_pairs, c, hash_reached){
    if(strcmp(hash, hash_reached) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_stateless_reached(mc_pair_stateless_t pair){

  if(reached_stateless(pair) == 0){
    MC_SET_RAW_MEM;
    char *hash = malloc(sizeof(char)*160) ;
    xbt_sha((char *)&pair, hash);
    xbt_dynar_push(reached_pairs, &hash); 
    MC_UNSET_RAW_MEM;
  }

}


void MC_ddfs_stateless_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS without visited state and with restore snapshot init");
  XBT_DEBUG("**************************************************");
 
  mc_pair_stateless_t mc_initial_pair;
  mc_state_t initial_graph_state;
  smx_process_t process; 
 
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_graph_state = MC_state_pair_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  reached_pairs = xbt_dynar_new(sizeof(char *), NULL); 

  MC_UNSET_RAW_MEM; 

  /* regarder dans l'automate toutes les transitions activables grâce à l'état initial du système 
    -> donnera les états initiaux de la propriété consistants avec l'état initial du système */

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pair_stateless_t), NULL);
  mc_pair_stateless_t pair_succ;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
  	if((res == 1) || (res == 2)){
	 
  	  MC_SET_RAW_MEM;

  	  mc_initial_pair = new_pair_stateless(initial_graph_state, transition_succ->dst);
  	  xbt_dynar_push(successors, &mc_initial_pair);

  	  MC_UNSET_RAW_MEM;

	}
      }
    }
  }

  cursor = 0;

  if(xbt_dynar_length(successors) == 0){
    xbt_dynar_foreach(a->states, cursor, state){
      if(state->type == -1){
	MC_SET_RAW_MEM;

	mc_initial_pair = new_pair_stateless(initial_graph_state, transition_succ->dst);
	xbt_dynar_push(successors, &mc_initial_pair);
	
	MC_UNSET_RAW_MEM;
      }
    }
  }

  cursor = 0;
  xbt_dynar_foreach(successors, cursor, pair_succ){
    MC_SET_RAW_MEM;

    xbt_fifo_unshift(mc_stack_liveness_stateless, pair_succ);

    MC_UNSET_RAW_MEM;

    if(cursor == 0){
      MC_ddfs_stateless(a, 0, 0);
    }else{
      MC_ddfs_stateless(a, 0, 1);
    }
  }  
}


void MC_ddfs_stateless(xbt_automaton_t a, int search_cycle, int restore){

 
}

