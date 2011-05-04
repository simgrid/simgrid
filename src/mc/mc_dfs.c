#include "private.h"
#include "unistd.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc,
                                "Logging specific to MC DFS algorithm");

xbt_dynar_t initial_pairs = NULL;
int max_pair = 0;
xbt_dynar_t visited_pairs;
xbt_dynar_t reached_pairs;

mc_pairs_t new_pair(mc_snapshot_t sn, mc_state_t sg, xbt_state_t st){
  mc_pairs_t p = NULL;
  p = xbt_new0(s_mc_pairs_t, 1);
  p->system_state = sn;
  p->automaton_state = st;
  p->graph_state = sg;
  p->num = max_pair;
  max_pair++;
  return p;
    
}

void set_pair_visited(mc_state_t gs, xbt_state_t as, int sc){

  if(visited(gs, as, sc) == 0){
    XBT_DEBUG("New visited pair : graph=%p, automaton=%p", gs, as);
    MC_SET_RAW_MEM;
    mc_visited_pairs_t p = NULL;
    p = xbt_new0(s_mc_visited_pairs_t, 1);
    p->graph_state = gs;
    p->automaton_state = as;
    p->search_cycle = sc;
    xbt_dynar_push(visited_pairs, &p);  
    MC_UNSET_RAW_MEM;
  }

  //XBT_DEBUG("New visited pair , length of visited pairs : %lu", xbt_dynar_length(visited_pairs));
}

int reached(mc_state_t gs, xbt_state_t as ){

  mc_reached_pairs_t rp = NULL;
  unsigned int cursor = 0;
  
  MC_SET_RAW_MEM;
  xbt_dynar_foreach(reached_pairs, cursor, rp){
    if((rp->graph_state == gs) &&(rp->automaton_state == as)){
      MC_UNSET_RAW_MEM;
      return 1;
    }
  }
  MC_UNSET_RAW_MEM;
  return 0;
}

void set_pair_reached(mc_state_t gs, xbt_state_t as){

  if(reached(gs, as) == 0){
    MC_SET_RAW_MEM;
    mc_reached_pairs_t p = NULL;
    p = xbt_new0(s_mc_reached_pairs_t, 1);
    p->graph_state = gs;
    p->automaton_state = as;
    xbt_dynar_push(reached_pairs, &p); 
    XBT_DEBUG("New reached pair : graph=%p, automaton=%p", gs, as);
    MC_UNSET_RAW_MEM;
  }

}

int visited(mc_state_t gs, xbt_state_t as, int sc){
  unsigned int c = 0;
  mc_visited_pairs_t p = NULL;
  unsigned int i;
  int different_process;

  MC_SET_RAW_MEM;
  xbt_dynar_foreach(visited_pairs, c, p){
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

  initial_graph_state = MC_state_new();
  xbt_swag_foreach(process, simix_global->process_list){
    if(MC_process_is_enabled(process)){
      MC_state_interleave_process(initial_graph_state, process);
    }
  }

  visited_pairs = xbt_dynar_new(sizeof(mc_visited_pairs_t), NULL); 
  reached_pairs = xbt_dynar_new(sizeof(mc_reached_pairs_t), NULL); 

  MC_take_snapshot(init_snapshot);

  MC_UNSET_RAW_MEM; 


  /* regarder dans l'automate toutes les transitions activables grâce à l'état initial du système 
    -> donnera les états initiaux de la propriété consistants avec l'état initial du système */

  unsigned int cursor = 0;
  unsigned int cursor2 = 0;
  xbt_state_t state = NULL;
  int res;
  xbt_transition_t transition_succ;
  xbt_dynar_t elses = xbt_dynar_new(sizeof(mc_pairs_t), NULL);
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pairs_t), NULL);
  //  int enabled_transition = 0;
  mc_pairs_t pair_succ;

  xbt_dynar_foreach(a->states, cursor, state){
    if(state->type == -1){
      xbt_dynar_foreach(state->out, cursor2, transition_succ){
	res = MC_automaton_evaluate_label(a, transition_succ->label);
	
	if(res == 1){
	 
	  MC_SET_RAW_MEM;

	  mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
	  xbt_dynar_push(successors, &mc_initial_pair);

	  //XBT_DEBUG("New initial successor");

	  MC_UNSET_RAW_MEM;

	}else{
	  if(res == 2){

	    MC_SET_RAW_MEM;
	    
	    mc_initial_pair = new_pair(init_snapshot, initial_graph_state, transition_succ->dst);
	    xbt_dynar_push(elses, &mc_initial_pair);

	    //XBT_DEBUG("New initial successor");

	    MC_UNSET_RAW_MEM;
	  }
	}
      }
    }
  }

  if(xbt_dynar_length(successors) > 0){

    cursor = 0;
    xbt_dynar_foreach(successors, cursor, pair_succ){
      MC_SET_RAW_MEM;

      xbt_fifo_unshift(mc_snapshot_stack, pair_succ);

      XBT_DEBUG("**************************************************");
      XBT_DEBUG("Initial state=%p ", pair_succ);

      MC_UNSET_RAW_MEM; 

      set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, 0);
   
      if(cursor == 0)
	MC_dfs(a, 0, 0);
      else
	MC_dfs(a, 0, 1);
    
    } 

  }else{

    if(xbt_dynar_length(elses) > 0){
      cursor = 0;
      xbt_dynar_foreach(elses, cursor, pair_succ){
	MC_SET_RAW_MEM;

	xbt_fifo_unshift(mc_snapshot_stack, pair_succ);

	XBT_DEBUG("**************************************************");
	XBT_DEBUG("Initial state=%p ", pair_succ);

	MC_UNSET_RAW_MEM;  

	set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, 0);

	if(cursor == 0)
	  MC_dfs(a, 0, 0);
	else
	  MC_dfs(a, 0, 1);
      } 
    }else{

      XBT_DEBUG("No initial state !");

      return;
    }
  }
  
}



void MC_dfs(xbt_automaton_t a, int search_cycle, int restore){


  if(xbt_fifo_size(mc_snapshot_stack) == 0)
    return;

  if(restore == 1){
    MC_restore_snapshot(((mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack)))->system_state);
    MC_UNSET_RAW_MEM;
  }


  //XBT_DEBUG("Lpv length after restore snapshot: %lu", xbt_dynar_length(visited_pairs));

  /* Get current state */
  mc_pairs_t current_pair = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("State=%p, %u interleave, mc_snapshot_stack size=%d", current_pair, MC_state_interleave_size(current_pair->graph_state), xbt_fifo_size(mc_snapshot_stack));
  //XBT_DEBUG("Restore : %d", restore);
  

  /* Update statistics */
  //mc_stats->visited_states++;
  //set_pair_visited(current_pair->graph_state, current_pair->automaton_state, search_cycle);
  
  //sleep(1);

  smx_process_t process = NULL;
  int value;
  mc_state_t next_graph_state = NULL;
  smx_req_t req = NULL;
  char *req_str;

  mc_pairs_t pair_succ;
  xbt_transition_t transition_succ;
  unsigned int cursor;
  int res;
  //int enabled_transition = 0;

  xbt_dynar_t elses = xbt_dynar_new(sizeof(mc_pairs_t), NULL);
  xbt_dynar_t successors = xbt_dynar_new(sizeof(mc_pairs_t), NULL); 

  mc_pairs_t next_pair;
  mc_snapshot_t next_snapshot;
  mc_snapshot_t current_snapshot;
  
  while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){

    XBT_DEBUG("Current pair : %p (%u interleave)", current_pair, MC_state_interleave_size(current_pair->graph_state)+1);
    //XBT_DEBUG("Visited pairs : %lu", xbt_dynar_length(visited_pairs));

    MC_SET_RAW_MEM;
    current_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(current_snapshot);
    MC_UNSET_RAW_MEM;
   
    
    /* Debug information */
    if(XBT_LOG_ISENABLED(mc_dfs, xbt_log_priority_debug)){
      req_str = MC_request_to_string(req, value);
      XBT_DEBUG("Execute: %s", req_str);
      xbt_free(req_str);
    }

    MC_state_set_executed_request(current_pair->graph_state, req, value);
    //mc_stats->executed_transitions++;

    /* Answer the request */
    SIMIX_request_pre(req, value); 

    /* Wait for requests (schedules processes) */
    MC_wait_for_requests();


    /* Create the new expanded graph_state */
    MC_SET_RAW_MEM;

    next_graph_state = MC_state_new();
    
    /* Get enabled process and insert it in the interleave set of the next graph_state */
    xbt_swag_foreach(process, simix_global->process_list){
      if(MC_process_is_enabled(process)){
	MC_state_interleave_process(next_graph_state, process);
      }
    }

    next_snapshot = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot(next_snapshot);
      
    MC_UNSET_RAW_MEM;

    xbt_dynar_reset(elses);
    xbt_dynar_reset(successors);
    
    cursor = 0;
    xbt_dynar_foreach(current_pair->automaton_state->out, cursor, transition_succ){

      res = MC_automaton_evaluate_label(a, transition_succ->label);

      MC_SET_RAW_MEM;
      next_pair = new_pair(next_snapshot,next_graph_state, transition_succ->dst);

      //XBT_DEBUG("Next pair : %p", next_pair);
	
      if(res == 1){ // enabled transition in automaton
	xbt_dynar_push(successors, &next_pair);   
	XBT_DEBUG("New Successors length : %lu", xbt_dynar_length(successors));
      }else{
	if(res == 2){
	  xbt_dynar_push(elses, &next_pair);
	  XBT_DEBUG("New Elses length : %lu", xbt_dynar_length(elses));
	}
      }

      MC_UNSET_RAW_MEM;
    }

   
    if((xbt_dynar_length(successors) == 0) && (xbt_dynar_length(elses) == 0) ){

      MC_SET_RAW_MEM;
      next_pair = new_pair(next_snapshot, next_graph_state, current_pair->automaton_state);
      xbt_dynar_push(successors, &next_pair);
      MC_UNSET_RAW_MEM;
	
    }

    //XBT_DEBUG("Successors length : %lu", xbt_dynar_length(successors));
    //XBT_DEBUG("Elses length : %lu", xbt_dynar_length(elses));

    if(xbt_dynar_length(successors) >0){

      cursor = 0;
      xbt_dynar_foreach(successors, cursor, pair_succ){

	//XBT_DEBUG("Search visited pair : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);
	
	if(visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle) == 0){

	  MC_SET_RAW_MEM;
	  xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
	  MC_UNSET_RAW_MEM;

	  set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle);
 
	  MC_dfs(a, search_cycle, 0);

	  if((search_cycle == 0) && (pair_succ->automaton_state->type == 1)){

	    mc_pairs_t first_item = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));
	    if((first_item->graph_state != pair_succ->graph_state) || (first_item->automaton_state != pair_succ->automaton_state) ){
	       MC_SET_RAW_MEM;
	      xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
	      MC_UNSET_RAW_MEM;
	    }

	    set_pair_reached(pair_succ->graph_state, pair_succ->automaton_state); 
	    XBT_DEBUG("Acceptance state : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);
	    MC_dfs(a, 1, 0);
	  }
	}else{
	  XBT_DEBUG("Pair (graph=%p, automaton=%p) already visited !", pair_succ->graph_state, pair_succ->automaton_state );
	  set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle);
	}
	
      }
    
    }else{ 

      cursor = 0;
      xbt_dynar_foreach(elses, cursor, pair_succ){

	//XBT_DEBUG("Search visited pair : graph=%p, automaton=%p", pair_succ->graph_state, pair_succ->automaton_state);
	
	if(visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle) == 0){

	  MC_SET_RAW_MEM;
	  xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
	  MC_UNSET_RAW_MEM;

	  set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle);
	  MC_dfs(a, search_cycle, 0);
	    
	  if((search_cycle == 0) && (pair_succ->automaton_state->type == 1)){

	    mc_pairs_t first_item = (mc_pairs_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_snapshot_stack));
	    if((first_item->graph_state != pair_succ->graph_state) || (first_item->automaton_state != pair_succ->automaton_state)){
	      MC_SET_RAW_MEM;
	      xbt_fifo_unshift(mc_snapshot_stack, pair_succ);
	      MC_UNSET_RAW_MEM;
	    }

	    set_pair_reached(pair_succ->graph_state, pair_succ->automaton_state); 
	    XBT_DEBUG("Acceptance state : graph state=%p, automaton state=%p",pair_succ->graph_state, pair_succ->automaton_state);
	    MC_dfs(a, 1, 0);
	  }
	}else{
	  XBT_DEBUG("Pair (graph=%p, automaton=%p) already visited !", pair_succ->graph_state, pair_succ->automaton_state );
	  set_pair_visited(pair_succ->graph_state, pair_succ->automaton_state, search_cycle);
	}
    
      }

    }  

    if(MC_state_interleave_size(current_pair->graph_state) > 0){
      MC_restore_snapshot(current_snapshot);
      MC_UNSET_RAW_MEM;
      XBT_DEBUG("Snapshot restored");
    }      
  }

  if((search_cycle == 1) && (reached(current_pair->graph_state, current_pair->automaton_state) == 1)){
    XBT_DEBUG("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    XBT_DEBUG("|             ACCEPTANCE CYCLE            |");
    XBT_DEBUG("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    // afficher chemin menant au cycle d'acceptation
    exit(0);
  }
  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_snapshot_stack);
  XBT_DEBUG("State shifted in snapshot_stack, mc_snapshot_stack size=%d", xbt_fifo_size(mc_snapshot_stack));
  MC_UNSET_RAW_MEM;

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
