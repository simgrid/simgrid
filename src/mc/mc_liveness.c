#include "private.h"
#include "unistd.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

xbt_dynar_t initial_pairs = NULL;
int max_pair = 0;
xbt_dynar_t visited_pairs;
xbt_dynar_t reached_pairs;
int expanded = 0;

mc_pairs_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pairs_t p = NULL;
  p = xbt_new0(s_mc_pairs_t, 1);
  p->system_state = sn;
  p->automaton_state = st;
  p->graph_state = sg;
  p->num = max_pair;
  max_pair++;
  mc_stats_pair->expanded_pairs++;
  return p;
    
}

void set_pair_visited(mc_state_t gs, xbt_state_t as, int sc){

  MC_SET_RAW_MEM;
  mc_visited_pairs_t p = NULL;
  p = xbt_new0(s_mc_visited_pairs_t, 1);
  p->graph_state = gs;
  p->automaton_state = as;
  p->search_cycle = sc;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  xbt_dynar_push(visited_pairs, &hash); 
  MC_UNSET_RAW_MEM;	

}

int reached(mc_state_t gs, xbt_state_t as ){

  /*mc_reached_pairs_t rp = NULL;
  unsigned int c= 0;
  unsigned int i;
  int different_process;

  MC_SET_RAW_MEM;
  xbt_dynar_foreach(reached_pairs, c, rp){
    if(rp->automaton_state == as){
      different_process=0;
      for(i=0; i < gs->max_pid; i++){
      	if(gs->proc_status[i].state != rp->graph_state->proc_status[i].state){
      	  different_process++;
      	}
      }
      if(different_process==0){
      	MC_UNSET_RAW_MEM;
      	XBT_DEBUG("Pair (graph=%p, automaton=%p(%s)) already reached (graph=%p)!", gs, as, as->id, rp->graph_state);
      	return 1;
      }
    }
  }
  MC_UNSET_RAW_MEM;
  return 0;*/
  //XBT_DEBUG("Search acceptance pair already reach !"); 

  char* hash_reached = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  mc_reached_pairs_t p = NULL;
  p = xbt_new0(s_mc_reached_pairs_t, 1);
  p->graph_state = gs;
  p->automaton_state = as;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  //XBT_DEBUG("Hash : %s", hash);
  xbt_dynar_foreach(reached_pairs, c, hash_reached){
    if(strcmp(hash, hash_reached) == 0){
      MC_UNSET_RAW_MEM;
      //XBT_DEBUG("Pair (graph=%p, automaton=%p(%s)) already reached !", gs, as, as->id);
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_reached(mc_state_t gs, xbt_state_t as){

  //XBT_DEBUG("Set pair (graph=%p, automaton=%p) reached ", gs, as);
  if(reached(gs, as) == 0){
    MC_SET_RAW_MEM;
    mc_reached_pairs_t p = NULL;
    p = xbt_new0(s_mc_reached_pairs_t, 1);
    p->graph_state = gs;
    p->automaton_state = as;
    char *hash = malloc(sizeof(char)*160) ;
    xbt_sha((char *)&p, hash);
    xbt_dynar_push(reached_pairs, &hash); 
    //XBT_DEBUG("New reached pair : graph=%p, automaton=%p(%s)", gs, as, as->id);
    MC_UNSET_RAW_MEM;
  }

}

int visited(mc_state_t gs, xbt_state_t as, int sc){
  /*unsigned int c = 0;
  mc_visited_pairs_t p = NULL;
  unsigned int i;
  int different_process;

  //XBT_DEBUG("Visited pair length : %lu", xbt_dynar_length(visited_pairs));

  MC_SET_RAW_MEM;
  xbt_dynar_foreach(visited_pairs, c, p){
    //XBT_DEBUG("Test pair visited");
    //sleep(1);
    if((p->automaton_state == as) && (p->search_cycle == sc)){
      different_process=0;
      for(i=0; i < gs->max_pid; i++){
	if(gs->proc_status[i].state != p->graph_state->proc_status[i].state){
	  different_process++;
	}
      }
      if(different_process==0){
	MC_UNSET_RAW_MEM;
	return 1;
      }
    }
  }
  MC_UNSET_RAW_MEM;
  return 0;*/

  //XBT_DEBUG("Test visited pair");

  char* hash_visited = malloc(sizeof(char)*160);
  unsigned int c= 0;

  MC_SET_RAW_MEM;
  mc_visited_pairs_t p = NULL;
  p = xbt_new0(s_mc_visited_pairs_t, 1);
  p->graph_state = gs;
  p->automaton_state = as;
  char *hash = malloc(sizeof(char)*160);
  xbt_sha((char *)&p, hash);
  //XBT_DEBUG("Hash : %s", hash);
  xbt_dynar_foreach(visited_pairs, c, hash_visited){
    if(strcmp(hash, hash_visited) == 0){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  
  MC_UNSET_RAW_MEM;
  //XBT_DEBUG("End test visited pair");
  return 0;

}

void MC_dfs_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("DFS init");
  XBT_DEBUG("**************************************************");
 
  mc_pairs_t mc_initial_pair;
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

  /* unsigned int cursor = 0; */
  /* xbt_state_t state = NULL; */
  /* int nb_init_state = 0; */

  /* xbt_dynar_foreach(a->states, cursor, state){ */
  /*   if(state->type == -1){ */

  /*     MC_SET_RAW_MEM; */
  /*     mc_initial_pair = new_pair(init_snapshot, initial_graph_state, state); */

  /*     xbt_fifo_unshift(mc_snapshot_stack, mc_initial_pair); */

  /*     XBT_DEBUG("**************************************************"); */
  /*     XBT_DEBUG("Initial state=%p ", mc_initial_pair); */

  /*     MC_UNSET_RAW_MEM; */

  /*     set_pair_visited(mc_initial_pair->graph_state, mc_initial_pair->automaton_state, 0); */
      
  /*     if(nb_init_state == 0) */
  /* 	MC_dfs(a, 0, 0); */
  /*     else */
  /* 	MC_dfs(a, 0, 1); */
      
  /*     nb_init_state++; */
  /*   } */
  /* } */

  /* regarder dans l'automate toutes les transitions activables grâce à l'état initial du système 
    -> donnera les états initiaux de la propriété consistants avec l'état initial du système */

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pairs_t), NULL);
  mc_pairs_t pair_succ;

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

    xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
    set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, 0);

    MC_UNSET_RAW_MEM;

    if(cursor == 0){
      MC_dfs(a, 0, 0);
    }else{
      MC_dfs(a, 0, 1);
    }
  }

 

 
  
}


void MC_dfs(xbt_automaton_t a, int search_cycle, int restore){

  smx_process_t process = NULL;


  if(xbt_fifo_size(mc_snapshot_stack) == 0)
    return;

  if(restore == 1){
    MC_restore_snapshot(((mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack)))->system_state);
    MC_UNSET_RAW_MEM;
  }


  /* Get current state */
  mc_pairs_t current_pair = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));

  if(restore==1){
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	//XBT_DEBUG("Pid : %lu", process->pid);
	MC_state_interleave_process(current_pair->graph_state, process);
      }
    }
  }

  XBT_DEBUG("************************************************** ( search_cycle = %d )", search_cycle);
  XBT_DEBUG("State : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));
  

  mc_stats_pair->visited_pairs++;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  mc_pairs_t pair_succ;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;
  //int enabled_transition = 0;

  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pairs_t), NULL);

  mc_pairs_t next_pair;
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

      if((search_cycle == 1) && (reached(pair_succ->graph_state, pair_succ->automaton_state) == 1)){
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("|             ACCEPTANCE CYCLE            |");
	XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	XBT_INFO("Counter-example that violates formula :");
	MC_show_snapshot_stack(mc_snapshot_stack);
	MC_dump_snapshot_stack(mc_snapshot_stack);
	MC_print_statistics_pairs(mc_stats_pair);
	exit(0);
      }
	
      if(visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle) == 0){

	//XBT_DEBUG("Unvisited pair !");

	mc_stats_pair->executed_transitions++;
 
	MC_SET_RAW_MEM;
	xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
	set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle);
	MC_UNSET_RAW_MEM;

	MC_dfs(a, search_cycle, 0);

	if((search_cycle == 0) && (current_pair->automaton_state->type == 1)){

	  MC_restore_snapshot(current_pair->system_state);
	  MC_UNSET_RAW_MEM;
 
	  xbt_swag_foreach(process, simix_global->process_list){
	    if(MC_process_is_enabled(process)){
	      //XBT_DEBUG("Pid : %lu", process->pid);
	      MC_state_interleave_process(current_pair->graph_state, process);
	    }
	  }
	    
	  set_pair_reached(current_pair->graph_state, current_pair->automaton_state);
	  XBT_DEBUG("Acceptance pair : graph=%p, automaton=%p(%s)", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id);
	  MC_dfs(a, 1, 1);

	}
      }else{

	XBT_DEBUG("Pair already visited !");

      }
    }
    
    
    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      MC_restore_snapshot(current_snapshot);
      MC_UNSET_RAW_MEM;
      //XBT_DEBUG("Snapshot restored");
    }
  }

  
  MC_SET_RAW_MEM;
  //remove_pair_reached(current_pair->graph_state);
  xbt_fifo_shift(mc_snapshot_stack);
  XBT_DEBUG("State (graph=%p, automaton =%p) shifted in snapshot_stack", current_pair->graph_state, current_pair->automaton_state);
  MC_UNSET_RAW_MEM;

}

void MC_show_snapshot_stack(xbt_fifo_t stack){
  int value;
  mc_pairs_t pair;
  xbt_fifo_item_t item;
  smx_req_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pairs_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if(req){
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}

void MC_dump_snapshot_stack(xbt_fifo_t stack){
  mc_pairs_t pair;

  MC_SET_RAW_MEM;
  while ((pair = (mc_pairs_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_delete(pair);
  MC_UNSET_RAW_MEM;
}

void MC_pair_delete(mc_pairs_t pair){
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


/********************* Double DFS using color pair *********************/






/********************* DPOR without replay *********************/



void MC_dpor_with_restore_snapshot_init(xbt_automaton_t a){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("DPOR (with restore snapshot) init");
  XBT_DEBUG("**************************************************");

  mc_pairs_t initial_pair;
  mc_state_t initial_graph_state;
  smx_process_t process; 
  mc_snapshot_t initial_snapshot;
  
  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_snapshot = xbt_new0(s_mc_snapshot_t, 1);

  initial_graph_state = MC_state_pair_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
      break;
    }
  }

  visited_pairs = xbt_dynar_new(sizeof(mc_visited_pairs_t), NULL); 
  reached_pairs = xbt_dynar_new(sizeof(mc_reached_pairs_t), NULL); 

  MC_take_snapshot(initial_snapshot);

  MC_UNSET_RAW_MEM; 

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
  	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
  	if((res == 1) || (res == 2)){
	 
  	  MC_SET_RAW_MEM;

  	  initial_pair = new_pair(initial_snapshot, initial_graph_state, transition_succ->dst);
	  xbt_fifo_unshift(mc_snapshot_stack, initial_pair);

  	  MC_UNSET_RAW_MEM;
	  
	  /* Add pair to list of visited pairs*/
	  set_pair_visited(initial_pair->graph_state, initial_pair->automaton_state, 0);

	  MC_dpor_with_restore_snapshot(a, 0, 0);

	  break;

  	}
      }
    }

    if(xbt_fifo_size(mc_snapshot_stack)>0)
      break;
  }

  /* Initial state of automaton is initial state for initial pair*/
  if(xbt_fifo_size(mc_snapshot_stack) == 0){

    cursor = 0;
    
    xbt_dynar_foreach(a->states, cursor, state){
      if(state->type == -1){
    
	MC_SET_RAW_MEM;

	initial_pair = new_pair(initial_snapshot, initial_graph_state, state);
	xbt_fifo_unshift(mc_snapshot_stack, initial_pair);
    
	MC_UNSET_RAW_MEM;
    
	/* Add pair to list of visited pairs*/
	set_pair_visited(initial_pair->graph_state, initial_pair->automaton_state, 0);
    
	MC_dpor_with_restore_snapshot(a, 0, 0);
	
	break;
      }
    }

  }

}


/* Test du dpor sans rejeu des états de la pile à partir de l'état initial du système mais avec restauration au milieu de l'arbre -> s'applique aux safety properties mais pas d'intégration pour les liveness */
void MC_dpor_with_restore_snapshot(xbt_automaton_t a, int search_cycle, int restore){

  smx_process_t process = NULL;
  
  if(xbt_fifo_size(mc_snapshot_stack) == 0)
    return;

  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL, prev_req = NULL;
  char *req_str;
  xbt_fifo_item_t item = NULL;

  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;

  mc_pairs_t next_pair;
  mc_snapshot_t next_snapshot;
  mc_snapshot_t current_snapshot;
  mc_pairs_t current_pair;
  mc_pairs_t prev_pair;
  int new_pair_created;

  while(xbt_fifo_size(mc_snapshot_stack) > 0){

    current_pair = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));

    
    XBT_DEBUG("**************************************************");
    XBT_DEBUG("Depth : %d, State : g=%p, a=%p(%s), %u interleave", xbt_fifo_size(mc_snapshot_stack),current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id,MC_state_interleave_size(current_pair->graph_state));
    
    mc_stats_pair->visited_pairs++;

    /* Add pair to list of visited pairs*/
    set_pair_visited(current_pair->graph_state, current_pair->automaton_state, search_cycle);

    if((xbt_fifo_size(mc_snapshot_stack) < MAX_DEPTH) && (req = MC_state_get_request(current_pair->graph_state, &value))){

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
      
      /* Get an enabled process and insert it in the interleave set of the next graph_state */
      xbt_swag_foreach(process, simix_global->process_list){
	if(MC_process_is_enabled(process)){
	  MC_state_interleave_process(next_graph_state, process);
	  break;
	}
      }

      next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
      MC_take_snapshot(next_snapshot);
      
      MC_UNSET_RAW_MEM;

      new_pair_created = 0;
      
      cursor = 0;
      xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
	res = MC_automaton_evaluate_label(a, transition_succ->label);

	if((res == 1) || (res == 2)){
	 
	  MC_SET_RAW_MEM;
	  next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);
	  xbt_fifo_unshift(mc_snapshot_stack, next_pair);
	  MC_UNSET_RAW_MEM;
	  
	  new_pair_created = 1;

	  break;
	 
	}
      }

      /* Pas d'avancement possible dans l'automate */
      if(new_pair_created == 0){ 
	  
	MC_SET_RAW_MEM;
	next_pair = new_pair(next_snapshot,next_graph_state,current_pair->automaton_state);
	xbt_fifo_unshift(mc_snapshot_stack, next_pair);
	MC_UNSET_RAW_MEM;

      }
      
    }else{
      XBT_DEBUG("There are no more processes to interleave.");
      
      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_snapshot_stack);
      MC_UNSET_RAW_MEM;

      while((current_pair = xbt_fifo_shift(mc_snapshot_stack)) != NULL){
	req = MC_state_get_internal_request(current_pair->graph_state);
	xbt_fifo_foreach(mc_snapshot_stack, item, prev_pair, mc_pairs_t) {
          if(MC_request_depend(req, MC_state_get_internal_request(prev_pair->graph_state))){
            if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
              XBT_DEBUG("Dependent Transitions:");
              prev_req = MC_state_get_executed_request(prev_pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, prev_pair->graph_state);
              xbt_free(req_str);
              prev_req = MC_state_get_executed_request(current_pair->graph_state, &value);
              req_str = MC_request_to_string(prev_req, value);
              XBT_DEBUG("%s (state=%p)", req_str, current_pair->graph_state);
              xbt_free(req_str);              
            }

            if(!MC_state_process_is_done(prev_pair->graph_state, req->issuer)){
              MC_state_interleave_process(prev_pair->graph_state, req->issuer);
	      
	    } else {
              XBT_DEBUG("Process %p is in done set", req->issuer);
	    }

            break;
          }
        }

	if(MC_state_interleave_size(current_pair->graph_state)){
	  MC_restore_snapshot(current_pair->system_state);
	  xbt_fifo_unshift(mc_snapshot_stack, current_pair);
	  XBT_DEBUG("Back-tracking to depth %d", xbt_fifo_size(mc_snapshot_stack));
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
