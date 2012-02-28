/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_liveness, mc,
                                "Logging specific to algorithms for liveness properties verification");

xbt_dynar_t reached_pairs;
xbt_dynar_t reached_pairs_hash;
xbt_dynar_t visited_pairs;
xbt_dynar_t visited_pairs_hash;
xbt_dynar_t successors;

xbt_dynar_t hosts_table;


/* fast implementation of djb2 algorithm */
unsigned int hash_region(char *str, int str_len){

  int c;
  register unsigned int hash = 5381;

  while (str_len--) {
    c = *str++;
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }

  return hash;

}

int snapshot_compare(mc_snapshot_t s1, mc_snapshot_t s2){

  
  if(s1->num_reg != s2->num_reg){
    XBT_DEBUG("Different num_reg (s1 = %u, s2 = %u)", s1->num_reg, s2->num_reg);
    return 1;
  }

  int i;
  int errors = 0;

  for(i=0 ; i< s1->num_reg ; i++){

    if(s1->regions[i]->type != s2->regions[i]->type){
      if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	XBT_DEBUG("Different type of region");
	errors++;
      }else{
	return 1;
      }
    }

    switch(s1->regions[i]->type){
      case 0:
      if(s1->regions[i]->size != s2->regions[i]->size){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different size of heap (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
	  errors++;
	}else{
	  return 1;
	}
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different start addr of heap (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
	  errors++;
	}else{
	  return 1;
	}
      }
      if(mmalloc_compare_heap(s1->regions[i]->data, s2->regions[i]->data)){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different heap (mmalloc_compare)");
	  errors++; 
	}else{
	  return 1;
	}
      }
      break;
      /*case 1 :
      if(s1->regions[i]->size != s2->regions[i]->size){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different size of libsimgrid (s1 = %zu, s2 = %zu)", s1->regions[i]->size, s2->regions[i]->size);
	  errors++;
	}else{
	  return 1;
	}
      }
      if(s1->regions[i]->start_addr != s2->regions[i]->start_addr){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different start addr of libsimgrid (s1 = %p, s2 = %p)", s1->regions[i]->start_addr, s2->regions[i]->start_addr);
 	  errors++;
	}else{
	  return 1;
	}
      }
      if(memcmp(s1->regions[i]->data, s2->regions[i]->data, s1->regions[i]->size) != 0){
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  XBT_DEBUG("Different memcmp for data in libsimgrid");
	  XBT_DEBUG("Size : %zu", s1->regions[i]->size);
 	  errors++;
	}else{
	  return 1;
	}
      }
      break;*/
    default:
      break;
    }
  }

  return (errors > 0);
  
  
}

int reached(xbt_state_t st){


  if(xbt_dynar_is_empty(reached_pairs)){

    return 0;

  }else{

    MC_SET_RAW_MEM;

    mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot_liveness(sn);    
    
    xbt_dynar_t prop_ato = xbt_dynar_new(sizeof(int), NULL);
    int res;
    int_f_void_t f;

    /* Get values of propositional symbols */
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
      f = (int_f_void_t)ps->function;
      res = (*f)();
      xbt_dynar_push_as(prop_ato, int, res);
    }

    cursor = 0;
    mc_pair_reached_t pair_test = NULL;

    //xbt_dict_t current_rdv_points = SIMIX_get_rdv_points();
     
    xbt_dynar_foreach(reached_pairs, cursor, pair_test){
      XBT_DEBUG("Pair reached #%u", cursor+1);
      if(automaton_state_compare(pair_test->automaton_state, st) == 0){
	if(propositional_symbols_compare_value(pair_test->prop_ato, prop_ato) == 0){
	  //XBT_DEBUG("Rdv points size %d - %d", xbt_dict_length(pair_test->rdv_points), xbt_dict_length(current_rdv_points));
	  //if(xbt_dict_length(pair_test->rdv_points) == xbt_dict_length(current_rdv_points)){
	  //if(rdv_points_compare(pair_test->rdv_points, current_rdv_points) == 0){
	      if(snapshot_compare(pair_test->system_state, sn) == 0){
		MC_free_snapshot(sn);
		xbt_dynar_reset(prop_ato);
		xbt_free(prop_ato);
		MC_UNSET_RAW_MEM;
		return 1;
	      }
	      /* }
	  }else{
	    XBT_DEBUG("Different size of rdv points (%d - %d)",xbt_dict_length(pair_test->rdv_points), xbt_dict_length(current_rdv_points) );
	    }*/
	}else{
	  XBT_DEBUG("Different values of propositional symbols");
	}
      }else{
	XBT_DEBUG("Different automaton state");
      }
    }

    MC_free_snapshot(sn);
    xbt_dynar_reset(prop_ato);
    xbt_free(prop_ato);
    MC_UNSET_RAW_MEM;
    return 0;
    
  }
}

int rdv_points_compare(xbt_dict_t d1, xbt_dict_t d2){ /* d1 = pair_test, d2 = current_pair */ 
  
   xbt_dict_cursor_t cursor_dict = NULL;
   char *key;
   char *data;
   smx_rdv_t rdv1, rdv2;
   xbt_fifo_item_t item1, item2;
   smx_action_t action1, action2;
   xbt_fifo_item_t item_req1, item_req2;
   smx_simcall_t req1, req2;
   int i=0;
   int j=0;

   xbt_dict_foreach(d1, cursor_dict, key, data){
     rdv1 = (smx_rdv_t)data;
     rdv2 = xbt_dict_get_or_null(d2, rdv1->name);
     if(rdv2 == NULL){
       XBT_DEBUG("Rdv point unknown");
       return 1;
     }else{
       if(xbt_fifo_size(rdv1->comm_fifo) != xbt_fifo_size(rdv2->comm_fifo)){
	 XBT_DEBUG("Different total of actions in mailbox \"%s\" (%d - %d)", rdv1->name, xbt_fifo_size(rdv1->comm_fifo),xbt_fifo_size(rdv2->comm_fifo) );
	 return 1;
       }else{
	 
	 XBT_DEBUG("Total of actions in mailbox \"%s\" : %d", rdv1->name, xbt_fifo_size(rdv1->comm_fifo)); 
	 
	 item1 = xbt_fifo_get_first_item(rdv1->comm_fifo);	
	 item2 = xbt_fifo_get_first_item(rdv2->comm_fifo);

	 while(i<xbt_fifo_size(rdv1->comm_fifo)){
	   action1 = (smx_action_t) xbt_fifo_get_item_content(item1);
	   action2 = (smx_action_t) xbt_fifo_get_item_content(item2);

	   if(action1->type != action2->type){
	     XBT_DEBUG("Different type of action");
	     return 1;
	   }

	   if(action1->state != action2->state){
	     XBT_DEBUG("Different state of action");
	     return 1;
	   }

	   if(xbt_fifo_size(action1->simcalls) != xbt_fifo_size(action2->simcalls)){
	     XBT_DEBUG("Different size of simcall list (%d - %d", xbt_fifo_size(action1->simcalls), xbt_fifo_size(action2->simcalls));
	     return 1;
	   }else{

	     item_req1 = xbt_fifo_get_first_item(action1->simcalls);	
	     item_req2 = xbt_fifo_get_first_item(action2->simcalls);

	     while(j<xbt_fifo_size(action1->simcalls)){

	       req1 = (smx_simcall_t) xbt_fifo_get_item_content(item_req1);
	       req2 = (smx_simcall_t) xbt_fifo_get_item_content(item_req2);
	       
	       if(req1->call != req2->call){
		 XBT_DEBUG("Different simcall call in simcalls of action (%d - %d)", (int)req1->call, (int)req2->call);
		 return 1;
	       }
	       if(req1->issuer->pid != req2->issuer->pid){
		 XBT_DEBUG("Different simcall issuer in simcalls of action (%lu- %lu)", req1->issuer->pid, req2->issuer->pid);
		 return 1;
	       }

	       item_req1 = xbt_fifo_get_next_item(item_req1);	
	       item_req2 = xbt_fifo_get_next_item(item_req2);
	       j++;
	       
	     }
	   }

	   switch(action1->type){
	   case 0: /* execution */
	   case 1: /* parallel execution */
	     if(strcmp(action1->execution.host->name, action2->execution.host->name) != 0)
	       return 1;
	     break;
	   case 2: /* comm */
	     if(action1->comm.type != action2->comm.type)
	       return 1;
	     //XBT_DEBUG("Type of comm : %d", action1->comm.type);
	     
	     switch(action1->comm.type){
	     case 0: /* SEND */
	       if(action1->comm.src_proc->pid != action2->comm.src_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.src_proc->smx_host->name, action2->comm.src_proc->smx_host->name) != 0)
		 return 1;
	       break;
	     case 1: /* RECEIVE */
	       if(action1->comm.dst_proc->pid != action2->comm.dst_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.dst_proc->smx_host->name, action2->comm.dst_proc->smx_host->name) != 0)
		 return 1;
	       break;
	     case 2: /* READY */
	       if(action1->comm.src_proc->pid != action2->comm.src_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.src_proc->smx_host->name, action2->comm.src_proc->smx_host->name) != 0)
		 return 1;
	       if(action1->comm.dst_proc->pid != action2->comm.dst_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.dst_proc->smx_host->name, action2->comm.dst_proc->smx_host->name) != 0)
		 return 1;
	       break;
	     case 3: /* DONE */
	       if(action1->comm.src_proc->pid != action2->comm.src_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.src_proc->smx_host->name, action2->comm.src_proc->smx_host->name) != 0)
		 return 1;
	       if(action1->comm.dst_proc->pid != action2->comm.dst_proc->pid)
		 return 1;
	       if(strcmp(action1->comm.dst_proc->smx_host->name, action2->comm.dst_proc->smx_host->name) != 0)
		 return 1;
	       break;
	       
	     } /* end of switch on type of comm */
	     
	     if(action1->comm.refcount != action2->comm.refcount)
	       return 1;
	     if(action1->comm.detached != action2->comm.detached)
	       return 1;
	     if(action1->comm.rate != action2->comm.rate)
	       return 1;
	     if(action1->comm.task_size != action2->comm.task_size)
	       return 1;
	     if(action1->comm.src_buff != action2->comm.src_buff)
	       return 1;
	     if(action1->comm.dst_buff != action2->comm.dst_buff)
	       return 1;
	     if(action1->comm.src_data != action2->comm.src_data)
	       return 1;
	     if(action1->comm.dst_data != action2->comm.dst_data)
	       return 1;
	     
	     break;
	   case 3: /* sleep */
	     if(strcmp(action1->sleep.host->name, action2->sleep.host->name) != 0)
	       return 1;
	     break;
	   case 4: /* synchro */
	     
	     break;
	   default:
	     break;
	   }

	   item1 = xbt_fifo_get_next_item(item1);	
	   item2 = xbt_fifo_get_next_item(item2);
	   i++;
	 }
       }
     }
   }

   return 0;
   
}

void set_pair_reached(xbt_state_t st){

 
  MC_SET_RAW_MEM;
  
  mc_pair_reached_t pair = NULL;
  pair = xbt_new0(s_mc_pair_reached_t, 1);
  pair->automaton_state = st;
  pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  pair->system_state = xbt_new0(s_mc_snapshot_t, 1);
  //pair->rdv_points = xbt_dict_new();  
  MC_take_snapshot_liveness(pair->system_state);

  /* Get values of propositional symbols */
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  int res;
  int_f_void_t f;

  xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(pair->prop_ato, int, res);
  }

  /*xbt_dict_t rdv_points = SIMIX_get_rdv_points();

  xbt_dict_cursor_t cursor_dict = NULL;
  char *key;
  char *data;
  xbt_fifo_item_t item;
  smx_action_t action;

  xbt_dict_foreach(rdv_points, cursor_dict, key, data){
    smx_rdv_t new_rdv = xbt_new0(s_smx_rvpoint_t, 1);
    new_rdv->name = strdup(((smx_rdv_t)data)->name);
    new_rdv->comm_fifo = xbt_fifo_new();
    xbt_fifo_foreach(((smx_rdv_t)data)->comm_fifo, item, action, smx_action_t) {
      smx_action_t a = xbt_new0(s_smx_action_t, 1);
      memcpy(a, action, sizeof(s_smx_action_t));
      xbt_fifo_push(new_rdv->comm_fifo, a);
      XBT_DEBUG("New action (type = %d, state = %d) in mailbox \"%s\"", action->type, action->state, key);
      if(action->type==2)
	XBT_DEBUG("Type of communication : %d, Ref count = %d", action->comm.type, action->comm.refcount);
    }
    //new_rdv->comm_fifo = xbt_fifo_copy(((smx_rdv_t)data)->comm_fifo);
    xbt_dict_set(pair->rdv_points, new_rdv->name, new_rdv, NULL);
    }*/
 
  xbt_dynar_push(reached_pairs, &pair); 

  MC_UNSET_RAW_MEM;
  
}


int reached_hash(xbt_state_t st){


  if(xbt_dynar_is_empty(reached_pairs_hash)){

    return 0;

  }else{

    MC_SET_RAW_MEM;

    mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot_liveness(sn);

    int j;
    unsigned int hash_regions[sn->num_reg];
    for(j=0; j<sn->num_reg; j++){
      hash_regions[j] = hash_region(sn->regions[j]->data, sn->regions[j]->size);
    }


    /* Get values of propositional symbols */
    xbt_dynar_t prop_ato = xbt_dynar_new(sizeof(int), NULL);
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    int res;
    int_f_void_t f;

    xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
      f = (int_f_void_t)ps->function;
      res = (*f)();
      xbt_dynar_push_as(prop_ato, int, res);
    }

    mc_pair_reached_hash_t pair_test = NULL;

    int region_diff = 0;

    cursor = 0;

    xbt_dynar_foreach(reached_pairs_hash, cursor, pair_test){

      if(automaton_state_compare(pair_test->automaton_state, st) == 0){
	if(propositional_symbols_compare_value(pair_test->prop_ato, prop_ato) == 0){
	  for(j=0 ; j< sn->num_reg ; j++){
	    if(hash_regions[j] != pair_test->hash_regions[j]){
	      region_diff++;
	    }
	  }
	  if(region_diff == 0){
	    MC_free_snapshot(sn);
	    xbt_dynar_reset(prop_ato);
	    xbt_free(prop_ato);
	    MC_UNSET_RAW_MEM;
	    return 1;
	  }else{
	    XBT_DEBUG("Different snapshot");
	  }
	}else{
	  XBT_DEBUG("Different values of propositional symbols");
	}
      }else{
	XBT_DEBUG("Different automaton state");
      }

      region_diff = 0;
    }
    
    MC_free_snapshot(sn);
    xbt_dynar_reset(prop_ato);
    xbt_free(prop_ato);
    MC_UNSET_RAW_MEM;
    return 0;
    
  }
}

void set_pair_reached_hash(xbt_state_t st){
 
  MC_SET_RAW_MEM;

  mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(sn);
 
  mc_pair_reached_hash_t pair = NULL;
  pair = xbt_new0(s_mc_pair_reached_hash_t, 1);
  pair->automaton_state = st;
  pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  pair->hash_regions = malloc(sizeof(unsigned int) * sn->num_reg);
  
  int i;

  for(i=0 ; i< sn->num_reg ; i++){
    pair->hash_regions[i] = hash_region(sn->regions[i]->data, sn->regions[i]->size);
  }
  
  /* Get values of propositional symbols */
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  int res;
  int_f_void_t f;

  xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(pair->prop_ato, int, res);
  }
  
  xbt_dynar_push(reached_pairs_hash, &pair);

  MC_free_snapshot(sn);
  
  MC_UNSET_RAW_MEM;
    
}


int visited(xbt_state_t st, int sc){


  if(xbt_dynar_is_empty(visited_pairs)){

    return 0;

  }else{

    MC_SET_RAW_MEM;

    mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot_liveness(sn);

    xbt_dynar_t prop_ato = xbt_dynar_new(sizeof(int), NULL);

    /* Get values of propositional symbols */
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    int res;
    int_f_void_t f;

    xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
      f = (int_f_void_t)ps->function;
      res = (*f)();
      xbt_dynar_push_as(prop_ato, int, res);
    }

    cursor = 0;
    mc_pair_visited_t pair_test = NULL;

    xbt_dynar_foreach(visited_pairs, cursor, pair_test){
      if(pair_test->search_cycle == sc) {
	if(automaton_state_compare(pair_test->automaton_state, st) == 0){
	  if(propositional_symbols_compare_value(pair_test->prop_ato, prop_ato) == 0){
	    if(snapshot_compare(pair_test->system_state, sn) == 0){
	    
	      MC_free_snapshot(sn);
	      xbt_dynar_reset(prop_ato);
	      xbt_free(prop_ato);
	      MC_UNSET_RAW_MEM;
		
	      return 1;
	
	    }else{
	      XBT_DEBUG("Different snapshot");
	    }
	  }else{
	    XBT_DEBUG("Different values of propositional symbols"); 
	  }
	}else{
	  XBT_DEBUG("Different automaton state");
	}
      }else{
	XBT_DEBUG("Different value of search_cycle");
      }
    }


    MC_free_snapshot(sn);
    xbt_dynar_reset(prop_ato);
    xbt_free(prop_ato);
    MC_UNSET_RAW_MEM;
    return 0;
    
  }
}


int visited_hash(xbt_state_t st, int sc){


  if(xbt_dynar_is_empty(visited_pairs_hash)){

    return 0;

  }else{

    MC_SET_RAW_MEM;

    mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
    MC_take_snapshot_liveness(sn);

    int j;
    unsigned int hash_regions[sn->num_reg];
    for(j=0; j<sn->num_reg; j++){
      hash_regions[j] = hash_region(sn->regions[j]->data, sn->regions[j]->size);
    }

    
    /* Get values of propositional symbols */
    xbt_dynar_t prop_ato = xbt_dynar_new(sizeof(int), NULL);
    unsigned int cursor = 0;
    xbt_propositional_symbol_t ps = NULL;
    int res;
    int_f_void_t f;

    xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
      f = (int_f_void_t)ps->function;
      res = (*f)();
      xbt_dynar_push_as(prop_ato, int, res);
    }

    mc_pair_visited_hash_t pair_test = NULL;

    int region_diff = 0;
    cursor = 0;

    xbt_dynar_foreach(visited_pairs_hash, cursor, pair_test){
  
      if(pair_test->search_cycle == sc) {
	if(automaton_state_compare(pair_test->automaton_state, st) == 0){
	  if(propositional_symbols_compare_value(pair_test->prop_ato, prop_ato) == 0){
	    for(j=0 ; j< sn->num_reg ; j++){
	      if(hash_regions[j] != pair_test->hash_regions[j]){
		region_diff++;
	      }
	    }
	    if(region_diff == 0){
	      MC_free_snapshot(sn);
	      xbt_dynar_reset(prop_ato);
	      xbt_free(prop_ato);
	      MC_UNSET_RAW_MEM;
	      return 1;
	    }else{
	      XBT_DEBUG("Different snapshot");
	    }
	  }else{
	    XBT_DEBUG("Different values of propositional symbols"); 
	  }
	}else{
	  XBT_DEBUG("Different automaton state");
	}
      }else{
	XBT_DEBUG("Different value of search_cycle");
      }
      
      region_diff = 0;
    }
    
    MC_free_snapshot(sn);
    xbt_dynar_reset(prop_ato);
    xbt_free(prop_ato);
    MC_UNSET_RAW_MEM;
    return 0;
    
  }
}

void set_pair_visited_hash(xbt_state_t st, int sc){
 
  MC_SET_RAW_MEM;

  mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(sn);
 
  mc_pair_visited_hash_t pair = NULL;
  pair = xbt_new0(s_mc_pair_visited_hash_t, 1);
  pair->automaton_state = st;
  pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  pair->search_cycle = sc;
  pair->hash_regions = malloc(sizeof(unsigned int) * sn->num_reg);
  
  int i;

  for(i=0 ; i< sn->num_reg ; i++){
    pair->hash_regions[i] = hash_region(sn->regions[i]->data, sn->regions[i]->size);
  }
  
  /* Get values of propositional symbols */
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  int res;
  int_f_void_t f;

  xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(pair->prop_ato, int, res);
  }
  
  xbt_dynar_push(visited_pairs_hash, &pair);

  MC_free_snapshot(sn);
  
  MC_UNSET_RAW_MEM;
    
}

void set_pair_visited(xbt_state_t st, int sc){

 
  MC_SET_RAW_MEM;
 
  mc_pair_visited_t pair = NULL;
  pair = xbt_new0(s_mc_pair_visited_t, 1);
  pair->automaton_state = st;
  pair->prop_ato = xbt_dynar_new(sizeof(int), NULL);
  pair->search_cycle = sc;
  pair->system_state = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(pair->system_state);


  /* Get values of propositional symbols */
  unsigned int cursor = 0;
  xbt_propositional_symbol_t ps = NULL;
  int res;
  int_f_void_t f;

  xbt_dynar_foreach(automaton->propositional_symbols, cursor, ps){
    f = (int_f_void_t)ps->function;
    res = (*f)();
    xbt_dynar_push_as(pair->prop_ato, int, res);
  }
  
  xbt_dynar_push(visited_pairs, &pair);
  
  MC_UNSET_RAW_MEM;
  
  
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
    xbt_dynar_foreach(automaton->propositional_symbols, cursor, p){
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

void MC_pair_stateless_delete(mc_pair_stateless_t pair){
  xbt_free(pair->graph_state->proc_status);
  xbt_free(pair->graph_state);
  xbt_free(pair);
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

void MC_ddfs_init(void){

  XBT_DEBUG("**************************************************");
  XBT_DEBUG("Double-DFS init");
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
  //reached_pairs_hash = xbt_dynar_new(sizeof(mc_pair_reached_hash_t), NULL);
  //visited_pairs = xbt_dynar_new(sizeof(mc_pair_visited_t), NULL);
  visited_pairs_hash = xbt_dynar_new(sizeof(mc_pair_visited_hash_t), NULL);
  successors = xbt_dynar_new(sizeof(mc_pair_stateless_t), NULL);

  /* Save the initial state */
  initial_snapshot_liveness = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(initial_snapshot_liveness);

  MC_UNSET_RAW_MEM; 

  unsigned int cursor = 0;
  xbt_state_t state;

  xbt_dynar_foreach(automaton->states, cursor, state){
    if(state->type == -1){
      
      MC_SET_RAW_MEM;
      mc_initial_pair = new_pair_stateless(initial_graph_state, state, MC_state_interleave_size(initial_graph_state));
      xbt_fifo_unshift(mc_stack_liveness, mc_initial_pair);
      MC_UNSET_RAW_MEM;
      
      if(cursor != 0){
	MC_restore_snapshot(initial_snapshot_liveness);
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
	//set_pair_reached_hash(state);

	if(cursor != 0){
	  MC_restore_snapshot(initial_snapshot_liveness);
	  MC_UNSET_RAW_MEM;
	}
	
	MC_ddfs(1);
	
      }
    }
  } 

}


void MC_ddfs(int search_cycle){

  smx_process_t process;
  mc_pair_stateless_t current_pair = NULL;

  if(xbt_fifo_size(mc_stack_liveness) == 0)
    return;


  /* Get current pair */
  current_pair = (mc_pair_stateless_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack_liveness));

  /* Update current state in buchi automaton */
  automaton->current_state = current_pair->automaton_state;

 
  XBT_DEBUG("********************* ( Depth = %d, search_cycle = %d )", xbt_fifo_size(mc_stack_liveness), search_cycle);
  XBT_DEBUG("Pair : graph=%p, automaton=%p(%s), %u interleave", current_pair->graph_state, current_pair->automaton_state, current_pair->automaton_state->id, MC_state_interleave_size(current_pair->graph_state));
 
  mc_stats_pair->visited_pairs++;

  sleep(1);

  int value;
  mc_state_t next_graph_state = NULL;
  smx_simcall_t req = NULL;
  char *req_str;

  xbt_transition_t transition_succ;
  unsigned int cursor = 0;
  int res;

  mc_pair_stateless_t next_pair = NULL;
  mc_pair_stateless_t pair_succ;

  if(xbt_fifo_size(mc_stack_liveness) < MAX_DEPTH_LIVENESS){

    //set_pair_visited(current_pair->automaton_state, search_cycle);
    set_pair_visited_hash(current_pair->automaton_state, search_cycle);

    //XBT_DEBUG("Visited pairs : %lu", xbt_dynar_length(visited_pairs));
    XBT_DEBUG("Visited pairs : %lu", xbt_dynar_length(visited_pairs_hash));

    if(current_pair->requests > 0){

      while((req = MC_state_get_request(current_pair->graph_state, &value)) != NULL){
   
	/* Debug information */
	if(XBT_LOG_ISENABLED(mc_liveness, xbt_log_priority_debug)){
	  req_str = MC_request_to_string(req, value);
	  XBT_DEBUG("Execute: %s", req_str);
	  xbt_free(req_str);
	}

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
		//if(reached_hash(pair_succ->automaton_state)){
	      
		XBT_DEBUG("Next pair (depth = %d, %u interleave) already reached !", xbt_fifo_size(mc_stack_liveness) + 1, MC_state_interleave_size(pair_succ->graph_state));

		XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
		XBT_INFO("|             ACCEPTANCE CYCLE            |");
		XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
		XBT_INFO("Counter-example that violates formula :");
		MC_show_stack_liveness(mc_stack_liveness);
		MC_dump_stack_liveness(mc_stack_liveness);
		MC_print_statistics_pairs(mc_stats_pair);
		exit(0);

	      }else{

		XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair : graph=%p, automaton=%p(%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->graph_state, pair_succ->automaton_state, pair_succ->automaton_state->id);
	      
		set_pair_reached(pair_succ->automaton_state);
		//set_pair_reached_hash(pair_succ->automaton_state);

		XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));
		//XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs_hash));

		MC_SET_RAW_MEM;
		xbt_fifo_unshift(mc_stack_liveness, pair_succ);
		MC_UNSET_RAW_MEM;
		
		MC_ddfs(search_cycle);

	      }

	    }else{

	      if(!visited_hash(pair_succ->automaton_state, search_cycle)){
		//if(!visited(pair_succ->automaton_state, search_cycle)){
		
		MC_SET_RAW_MEM;
		xbt_fifo_unshift(mc_stack_liveness, pair_succ);
		MC_UNSET_RAW_MEM;
		
		MC_ddfs(search_cycle);
		
	      }else{

		XBT_DEBUG("Next pair already visited ! ");

	      }
	      
	    }

	  }else{
	  
	    if(((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

	      XBT_DEBUG("Next pair (depth =%d) -> Acceptance pair : graph=%p, automaton=%p(%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->graph_state, pair_succ->automaton_state, pair_succ->automaton_state->id);
	    
	      set_pair_reached(pair_succ->automaton_state); 
	      //set_pair_reached_hash(pair_succ->automaton_state);

	      search_cycle = 1;

	      XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));
	      //XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs_hash));

	    }

	    if(!visited_hash(pair_succ->automaton_state, search_cycle)){
	      //if(!visited(pair_succ->automaton_state, search_cycle)){
	      
	      MC_SET_RAW_MEM;
	      xbt_fifo_unshift(mc_stack_liveness, pair_succ);
	      MC_UNSET_RAW_MEM;
	      
	      MC_ddfs(search_cycle);
	      
	    }else{

	      XBT_DEBUG("Next pair already visited ! ");

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

 
    }else{  /*No request to execute, search evolution in Büchi automaton */

      MC_SET_RAW_MEM;

      /* Create the new expanded graph_state */
      next_graph_state = MC_state_pair_new();

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
	      //if(reached_hash(pair_succ->automaton_state)){

	      XBT_DEBUG("Next pair (depth = %d) already reached !", xbt_fifo_size(mc_stack_liveness) + 1);

	      XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	      XBT_INFO("|             ACCEPTANCE CYCLE            |");
	      XBT_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	      XBT_INFO("Counter-example that violates formula :");
	      MC_show_stack_liveness(mc_stack_liveness);
	      MC_dump_stack_liveness(mc_stack_liveness);
	      MC_print_statistics_pairs(mc_stats_pair);
	      exit(0);

	    }else{

	      XBT_DEBUG("Next pair (depth = %d) -> Acceptance pair : graph=%p, automaton=%p(%s)", xbt_fifo_size(mc_stack_liveness) + 1, pair_succ->graph_state, pair_succ->automaton_state, pair_succ->automaton_state->id);
	      
	      set_pair_reached(pair_succ->automaton_state);
	      //set_pair_reached_hash(pair_succ->automaton_state);
		
	      XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));
	      //XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs_hash));

	      MC_SET_RAW_MEM;
	      xbt_fifo_unshift(mc_stack_liveness, pair_succ);
	      MC_UNSET_RAW_MEM;
	      
	      MC_ddfs(search_cycle);

	    }

	  }else{

	    if(!visited_hash(pair_succ->automaton_state, search_cycle)){
	      //if(!visited(pair_succ->automaton_state, search_cycle)){

	      MC_SET_RAW_MEM;
	      xbt_fifo_unshift(mc_stack_liveness, pair_succ);
	      MC_UNSET_RAW_MEM;
	      
	      MC_ddfs(search_cycle);
	      
	    }else{

	      XBT_DEBUG("Next pair already visited ! ");

	    }
	  }
	    

	}else{
	    
	  if(((pair_succ->automaton_state->type == 1) || (pair_succ->automaton_state->type == 2))){

	    set_pair_reached(pair_succ->automaton_state);
	    //set_pair_reached_hash(pair_succ->automaton_state);
	    	    
	    search_cycle = 1;

	    XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs));
	    //XBT_DEBUG("Reached pairs : %lu", xbt_dynar_length(reached_pairs_hash));

	  }

	  if(!visited_hash(pair_succ->automaton_state, search_cycle)){
	    //if(!visited(pair_succ->automaton_state, search_cycle)){

	    MC_SET_RAW_MEM;
	    xbt_fifo_unshift(mc_stack_liveness, pair_succ);
	    MC_UNSET_RAW_MEM;
	    
	    MC_ddfs(search_cycle);
	    
	  }else{

	    XBT_DEBUG("Next pair already visited ! ");

	  }

	}

	/* Restore system before checking others successors */
	if(cursor != xbt_dynar_length(successors) - 1)
	  MC_replay_liveness(mc_stack_liveness, 1);

	 
      }
           
    }
    
  }else{
    
    XBT_DEBUG("Max depth reached");

  }

  if(xbt_fifo_size(mc_stack_liveness) == MAX_DEPTH_LIVENESS ){
    XBT_DEBUG("Pair (graph=%p, automaton =%p, search_cycle = %d, depth = %d) shifted in stack, maximum depth reached", current_pair->graph_state, current_pair->automaton_state, search_cycle, xbt_fifo_size(mc_stack_liveness) );
  }else{
    XBT_DEBUG("Pair (graph=%p, automaton =%p, search_cycle = %d, depth = %d) shifted in stack", current_pair->graph_state, current_pair->automaton_state, search_cycle, xbt_fifo_size(mc_stack_liveness) );
  }

  
  MC_SET_RAW_MEM;
  xbt_fifo_shift(mc_stack_liveness);
  if((current_pair->automaton_state->type == 1) || (current_pair->automaton_state->type == 2)){
    xbt_dynar_pop(reached_pairs, NULL);
    //xbt_dynar_pop(reached_pairs_hash, NULL);
  }
  MC_UNSET_RAW_MEM;
  
  

}
