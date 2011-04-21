#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc,
                                "Logging specific to MC DFS algorithm");


extern xbt_fifo_t mc_snapshot_stack; 
mc_pairs_t mc_reached = NULL;
xbt_dynar_t initial_pairs = NULL;
int max_pair = 0;
xbt_dynar_t list_pairs_reached = NULL;
extern mc_snapshot_t initial_snapshot;

mc_pairs_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pairs_t p = NULL;
  p = xbt_new0(s_mc_pairs_t, 1);
  p->system_state = sn;
  p->automaton_state = st;
  p->graph_state = sg;
  p->visited = 0;
  p->num = max_pair;
  max_pair++;
  return p;
    
}

void set_pair_visited(mc_pairs_t p){
  p->visited = 1;
}

int pair_reached(xbt_dynar_t p, int num_pair){
  int n;
  unsigned int cursor = 0;
  xbt_dynar_foreach(p, cursor, n){
    if(n == num_pair)
      return 1;
  }
  return 0;
}

void MC_dfs_init(xbt_automaton_t a){

  mc_pairs_t mc_initial_pair = NULL;
  mc_state_t initial_graph_state = NULL;
  smx_process_t process;

  MC_wait_for_requests();

  MC_SET_RAW_MEM;

  initial_snapshot = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_snapshot);
  initial_graph_state = MC_state_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  list_pairs_reached = xbt_dynar_new(sizeof(int), NULL); 

  MC_UNSET_RAW_MEM;


  //regarder dans l'automate toutes les transitions activables grâce à l'état initial du système -> donnera les états initiaux de la propriété consistants avec l'état initial du système

  unsigned int cursor = 0;
  xbt_state_t state = NULL;
  unsigned int cursor2 = 0;
  int res;
  xbt_transition_t transition_succ = NULL;
  xbt_dynar_t elses = xbt_dynar_new(sizeof(xbt_transition_t), NULL);
  int enabled_transition = 0;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
	
	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
	if(res == 1){
	 
	  MC_SET_RAW_MEM;

	  mc_initial_pair = new_pair(initial_snapshot, initial_graph_state, transition_succ->dst);
	  xbt_fifo_unshift(mc_snapshot_stack, mc_initial_pair);
	  set_pair_visited(mc_initial_pair);

	  MC_UNSET_RAW_MEM;

	  enabled_transition = 1;

	  XBT_DEBUG(" ++++++++++++++++++ new initial pair +++++++++++++++++++");

	  MC_dfs(a, 0);
	}else{
	  if(res == 2){
	    xbt_dynar_push(elses,&transition_succ);
	  }
	}
      }
    }
  }

  if(enabled_transition == 0){
    cursor2 = 0;
    xbt_dynar_foreach(elses, cursor, transition_succ){
    
      MC_SET_RAW_MEM;

      mc_initial_pair = new_pair(initial_snapshot, initial_graph_state, transition_succ->dst);
      xbt_fifo_unshift(mc_snapshot_stack, mc_initial_pair);
      set_pair_visited(mc_initial_pair);
    
      MC_UNSET_RAW_MEM;
    
      MC_dfs(a, 0);
    }
  }
  
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

/*void MC_dfs(xbt_automaton_t a, int search_cycle){

  xbt_state_t current_state = (xbt_state_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(stack_automaton_dfs));
  xbt_transition_t transition_succ = NULL;
  xbt_state_t successor = NULL;
  unsigned int cursor = 0;
  xbt_dynar_t elses = xbt_dynar_new(sizeof(xbt_transition_t), NULL);
  int res;

  //printf("++ current state : %s\n", current_state->id);

  xbt_dynar_foreach(current_state->out, cursor, transition_succ){
    successor = transition_succ->dst; 
    res = MC_automaton_evaluate_label(a, transition_succ->label);
    //printf("-- state : %s, transition_label : %d, res = %d\n", successor->id, transition_succ->label->type, res);
    if(res == 1){
      if(search_cycle == 1 && reached_dfs != NULL){
	if(strcmp(reached_dfs->id, successor->id) == 0){
	  xbt_dynar_push(state_automaton_visited_dfs, &successor); 
	  printf("\n-*-*-*-*-*-*- ACCEPTANCE CYCLE -*-*-*-*-*-*-\n");
	  printf("Visited states : \n");
	  unsigned int cursor2 = 0;
	  xbt_state_t visited_state = NULL;
	  xbt_dynar_foreach(state_automaton_visited_dfs, cursor2, visited_state){
	    printf("State : %s\n", visited_state->id);
	  }
	  exit(1);
	}
      }
      if(successor->visited == 0){
	successor->visited = 1;
	xbt_fifo_unshift(stack_automaton_dfs, successor);
	xbt_dynar_push(state_automaton_visited_dfs, &successor);      
	MC_dfs(a, search_cycle);
	if(search_cycle == 0 && successor->type == 1){
	  reached_dfs = successor;
	  MC_dfs(a, 1);
	}
      }
    }else{
      if(res == 2){
	xbt_dynar_push(elses,&transition_succ);
      }
    }
  }
  
  cursor = 0;
  xbt_dynar_foreach(elses, cursor, transition_succ){
   successor = transition_succ->dst; 
   if(search_cycle == 1 && reached_dfs != NULL){
     if(strcmp(reached_dfs->id, successor->id) == 0){
       xbt_dynar_push(state_automaton_visited_dfs, &successor); 
       printf("\n-*-*-*-*-*-*- ACCEPTANCE CYCLE -*-*-*-*-*-*-\n");
       printf("Visited states : \n");
       unsigned int cursor2 = 0;
       xbt_state_t visited_state = NULL;
       xbt_dynar_foreach(state_automaton_visited_dfs, cursor2, visited_state){
	 printf("State : %s\n", visited_state->id);
       }
       exit(1);
     }
   }
   if(successor->visited == 0){
     successor->visited = 1;
     xbt_fifo_unshift(stack_automaton_dfs, successor);
     xbt_dynar_push(state_automaton_visited_dfs, &successor);      
     MC_dfs(a, search_cycle);
     if(search_cycle == 0 && successor->type == 1){
       reached_dfs = successor;
       MC_dfs(a, 1);
     }
   }
  }

  xbt_fifo_shift(stack_automaton_dfs);
  }*/


void MC_dfs(xbt_automaton_t a, int search_cycle){

  smx_process_t process = NULL;
  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  xbt_transition_t transition_succ = NULL;
  xbt_state_t successor = NULL;
  unsigned int cursor = 0;
  xbt_dynar_t elses = xbt_dynar_new(sizeof(xbt_transition_t), NULL);
  int res;
  int enabled_transition = 0;

  mc_pairs_t next_pair = NULL;
  mc_snapshot_t next_snapshot = NULL;
  

  /* Get current state */
  mc_pairs_t current_pair = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("State=%p, %u interleave",current_pair->graph_state,MC_state_interleave_size(current_pair->graph_state));
  
  /* Update statistics */
  mc_stats->visited_states++;

  MC_restore_snapshot(current_pair->system_state);
  MC_UNSET_RAW_MEM;

  //FIXME : vérifier condition permettant d'avoir tous les successeurs possibles dans le graph

  while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){
    
    /* Debug information */
    if(XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_debug)){
      req_str = MC_request_to_string(req, value);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);
    }

    MC_state_set_executed_request(current_pair->graph_state, req, value);
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
      }
    }

    next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(next_snapshot);

    MC_UNSET_RAW_MEM;

    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){
      successor = transition_succ->dst;
      res = MC_automaton_evaluate_label(a, transition_succ->label);
      if(res == 1){ // enabled transition 
	
	MC_SET_RAW_MEM;
	next_pair = new_pair(next_snapshot, next_graph_state, successor);
     
	if((search_cycle == 1) && (pair_reached(list_pairs_reached, next_pair->num) == 1)){
	  printf("\n-*-*-*-*-*-*- ACCEPTANCE CYCLE -*-*-*-*-*-*-\n");
	  // afficher chemin menant au cycle d'acceptation
	  exit(1);
	}
	if(next_pair->visited == 0){ // FIXME : tester avec search_cycle ?
	  xbt_fifo_unshift(mc_snapshot_stack, next_pair);
	  set_pair_visited(next_pair);
	  MC_dfs(a, search_cycle);
	  if((search_cycle == 0) && (next_pair->automaton_state->type == 1)){
	    xbt_dynar_push(list_pairs_reached, &next_pair->num);
	    MC_dfs(a, 1);
	  }
	}

	MC_UNSET_RAW_MEM;
	enabled_transition = 1;
      }else{
	if(res == 2){
	  xbt_dynar_push(elses,&transition_succ);
	}
      }
    }

    if(enabled_transition == 0){
      cursor = 0;
      xbt_dynar_foreach(elses, cursor, transition_succ){
	successor = transition_succ->dst;
	MC_SET_RAW_MEM;
	next_pair = new_pair(next_snapshot, next_graph_state, successor);
	
	if((search_cycle == 1) && (pair_reached(list_pairs_reached, next_pair->num) == 1)){
	  printf("\n-*-*-*-*-*-*- ACCEPTANCE CYCLE -*-*-*-*-*-*-\n");
	  // afficher chemin menant au cycle d'acceptation
	  exit(1);
	}
	if(next_pair->visited == 0){ // FIXME : tester avec search_cycle ?
	  xbt_fifo_unshift(mc_snapshot_stack, next_pair);
	  set_pair_visited(next_pair);
	  MC_dfs(a, search_cycle);
	  if((search_cycle == 0) && (next_pair->automaton_state->type == 1)){
	    xbt_dynar_push(list_pairs_reached, &next_pair->num);
	    MC_dfs(a, 1);
	  }
	}

	MC_UNSET_RAW_MEM;
      }
    }
    
  }

  xbt_fifo_shift(mc_snapshot_stack);

}
