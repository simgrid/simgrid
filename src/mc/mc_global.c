/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "xbt/fifo.h"
#include "mc_private.h"
#include "xbt/automaton.h"

XBT_LOG_NEW_CATEGORY(mc, "All MC categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

/* Configuration support */
e_mc_reduce_t mc_reduce_kind=e_mc_reduce_unset;

extern int _surf_init_status;
void _mc_cfg_cb_reduce(const char *name, int pos) {
  if (_surf_init_status && !_surf_do_model_check) {
    xbt_die("You are specifying a reduction strategy after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  char *val= xbt_cfg_get_string(_surf_cfg_set, name);
  if (!strcasecmp(val,"none")) {
    mc_reduce_kind = e_mc_reduce_none;
  } else if (!strcasecmp(val,"dpor")) {
    mc_reduce_kind = e_mc_reduce_dpor;
  } else {
    xbt_die("configuration option %s can only take 'none' or 'dpor' as a value",name);
  }
  xbt_cfg_set_int(_surf_cfg_set,"model-check",1);
}

void _mc_cfg_cb_checkpoint(const char *name, int pos) {
  if (_surf_init_status && !_surf_do_model_check) {
    xbt_die("You are specifying a checkpointing value after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _surf_mc_checkpoint = xbt_cfg_get_int(_surf_cfg_set, name);
  xbt_cfg_set_int(_surf_cfg_set,"model-check",1);
}
void _mc_cfg_cb_property(const char *name, int pos) {
  if (_surf_init_status && !_surf_do_model_check) {
    xbt_die("You are specifying a property after the initialization (through MSG_config?), but model-checking was not activated at config time (through --cfg=model-check:1). This won't work, sorry.");
  }
  _surf_mc_property_file= xbt_cfg_get_string(_surf_cfg_set, name);
  xbt_cfg_set_int(_surf_cfg_set,"model-check",1);
}


/* MC global data structures */

mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;
double *mc_time = NULL;
mc_snapshot_t initial_snapshot = NULL;
int raw_mem_set;

/* Safety */

xbt_fifo_t mc_stack_safety = NULL;
mc_stats_t mc_stats = NULL;

/* Liveness */

mc_stats_pair_t mc_stats_pair = NULL;
xbt_fifo_t mc_stack_liveness = NULL;
mc_snapshot_t initial_snapshot_liveness = NULL;

xbt_automaton_t _mc_property_automaton = NULL;

static void MC_assert_pair(int prop);

void MC_do_the_modelcheck_for_real() {
  if (!_surf_mc_property_file || _surf_mc_property_file[0]=='\0') {
    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_dpor;

    XBT_INFO("Check a safety property");
    MC_modelcheck();

  } else  {

    if (mc_reduce_kind==e_mc_reduce_unset)
      mc_reduce_kind=e_mc_reduce_none;

    XBT_INFO("Check the liveness property %s",_surf_mc_property_file);
    MC_automaton_load(_surf_mc_property_file);
    MC_modelcheck_liveness();
  }
}

/**
 *  \brief Initialize the model-checker data structures
 */
void MC_init_safety(void)
{

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  /* Check if MC is already initialized */
  if (initial_snapshot)
    return;

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */
  
  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack_safety = xbt_fifo_new();

  MC_UNSET_RAW_MEM;

  MC_dpor_init();

  MC_SET_RAW_MEM;
  /* Save the initial state */
  initial_snapshot = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_snapshot);
  MC_UNSET_RAW_MEM;


  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


void MC_modelcheck(void)
{
  MC_init_safety();
  MC_dpor();
  MC_exit();
}

void MC_modelcheck_liveness(){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  /* init stuff */
  XBT_DEBUG("Start init mc");
  
  mc_time = xbt_new0(double, simix_process_maxpid);

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats_pair = xbt_new0(s_mc_stats_pair_t, 1);

  XBT_DEBUG("Creating stack");

  /* Create exploration stack */
  mc_stack_liveness = xbt_fifo_new();

  MC_UNSET_RAW_MEM;


  MC_ddfs_init();

  /* We're done */
  MC_print_statistics_pairs(mc_stats_pair);
  xbt_free(mc_time);
  MC_memory_exit();
  exit(0);
}


void MC_exit(void)
{
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);
  MC_memory_exit();
}


int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value;*/
  return 0;
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  smx_process_t process;
  smx_simcall_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->simcall;
      if (req->call != SIMCALL_NONE && !MC_request_is_visible(req))
        SIMIX_simcall_pre(req, 0);
    }
  }
}

int MC_deadlock_check()
{
  int deadlock = FALSE;
  smx_process_t process;
  if(xbt_swag_size(simix_global->process_list)){
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list){
      if(process->simcall.call != SIMCALL_NONE
         && MC_request_is_enabled(&process->simcall)){
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}

/**
 * \brief Re-executes from the state at position start all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
 * \param start Start index to begin the re-execution.
 */
void MC_replay(xbt_fifo_t stack, int start)
{
  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  int value, i = 1;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item, start_item;
  mc_state_t state;

  XBT_DEBUG("**** Begin Replay ****");

  if(start == -1){
    /* Restore the initial state */
    MC_restore_snapshot(initial_snapshot);
    /* At the moment of taking the snapshot the raw heap was set, so restoring
     * it will set it back again, we have to unset it to continue  */
    MC_UNSET_RAW_MEM;
  }

  start_item = xbt_fifo_get_last_item(stack);
  if(start != -1){
    while (i != start){
      start_item = xbt_fifo_get_prev_item(start_item);
      i++;
    }
  }

  /* Traverse the stack from the state at position start and re-execute the transitions */
  for (item = start_item;
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
   
    if(saved_req){
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->simcall;

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }
    }
         
    SIMIX_simcall_pre(req, value);
    MC_wait_for_requests();
         
    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;
  }
  XBT_DEBUG("**** End Replay ****");

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  

}

void MC_replay_liveness(xbt_fifo_t stack, int all_stack)
{

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  int value;
  char *req_str;
  smx_simcall_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;
  mc_pair_stateless_t pair;
  int depth = 1;

  XBT_DEBUG("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_snapshot_liveness);
  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  MC_UNSET_RAW_MEM;

  if(all_stack){

    item = xbt_fifo_get_last_item(stack);

    while(depth <= xbt_fifo_size(stack)){

      pair = (mc_pair_stateless_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats_pair->visited_pairs++;

      item = xbt_fifo_get_prev_item(item);
    }

  }else{

    /* Traverse the stack from the initial state and re-execute the transitions */
    for (item = xbt_fifo_get_last_item(stack);
         item != xbt_fifo_get_first_item(stack);
         item = xbt_fifo_get_prev_item(item)) {

      pair = (mc_pair_stateless_t) xbt_fifo_get_item_content(item);
      state = (mc_state_t) pair->graph_state;

      if(pair->requests > 0){
   
        saved_req = MC_state_get_executed_request(state, &value);
        //XBT_DEBUG("SavedReq->call %u", saved_req->call);
      
        if(saved_req != NULL){
          /* because we got a copy of the executed request, we have to fetch the  
             real one, pointed by the request field of the issuer process */
          req = &saved_req->issuer->simcall;
          //XBT_DEBUG("Req->call %u", req->call);
  
          /* Debug information */
          if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
            req_str = MC_request_to_string(req, value);
            XBT_DEBUG("Replay (depth = %d) : %s (%p)", depth, req_str, state);
            xbt_free(req_str);
          }
  
        }
 
        SIMIX_simcall_pre(req, value);
        MC_wait_for_requests();
      }

      depth++;
    
      /* Update statistics */
      mc_stats_pair->visited_pairs++;
    }
  }  

  XBT_DEBUG("**** End Replay ****");

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
 */
void MC_dump_stack_safety(xbt_fifo_t stack)
{
  
  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_show_stack_safety(stack);

  if(!_surf_mc_checkpoint){

    mc_state_t state;

    MC_SET_RAW_MEM;
    while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
      MC_state_delete(state);
    MC_UNSET_RAW_MEM;

  }

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}


void MC_show_stack_safety(xbt_fifo_t stack)
{
  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state, &value);
    if(req){
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}

void MC_show_deadlock(smx_simcall_t req)
{
  /*char *req_str = NULL;*/
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
    XBT_INFO("%s", req_str);
    xbt_free(req_str);*/
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack_safety(mc_stack_safety);
}


void MC_show_stack_liveness(xbt_fifo_t stack){
  int value;
  mc_pair_stateless_t pair;
  xbt_fifo_item_t item;
  smx_simcall_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (pair = (mc_pair_stateless_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(pair->graph_state, &value);
    if(req){
      if(pair->requests>0){
        req_str = MC_request_to_string(req, value);
        XBT_INFO("%s", req_str);
        xbt_free(req_str);
      }else{
        XBT_INFO("End of system requests but evolution in Büchi automaton");
      }
    }
  }
}

void MC_dump_stack_liveness(xbt_fifo_t stack){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  mc_pair_stateless_t pair;

  MC_SET_RAW_MEM;
  while ((pair = (mc_pair_stateless_t) xbt_fifo_pop(stack)) != NULL)
    MC_pair_stateless_delete(pair);
  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;

}


void MC_print_statistics(mc_stats_t stats)
{
  //XBT_INFO("State space size ~= %lu", stats->state_size);
  XBT_INFO("Expanded states = %lu", stats->expanded_states);
  XBT_INFO("Visited states = %lu", stats->visited_states);
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  XBT_INFO("Expanded / Visited = %lf",
           (double) stats->visited_states / stats->expanded_states);
  /*XBT_INFO("Exploration coverage = %lf",
    (double)stats->expanded_states / stats->state_size); */
}

void MC_print_statistics_pairs(mc_stats_pair_t stats)
{
  XBT_INFO("Expanded pairs = %lu", stats->expanded_pairs);
  XBT_INFO("Visited pairs = %lu", stats->visited_pairs);
  //XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  XBT_INFO("Expanded / Visited = %lf",
           (double) stats->visited_pairs / stats->expanded_pairs);
  /*XBT_INFO("Exploration coverage = %lf",
    (double)stats->expanded_states / stats->state_size); */
}

void MC_assert(int prop)
{
  if (MC_IS_ENABLED && !prop){
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    MC_dump_stack_safety(mc_stack_safety);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

static void MC_assert_pair(int prop){
  if (MC_IS_ENABLED && !prop) {
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    //XBT_INFO("Counter-example execution trace:");
    MC_show_stack_liveness(mc_stack_liveness);
    //MC_dump_snapshot_stack(mc_snapshot_stack);
    MC_print_statistics_pairs(mc_stats_pair);
    xbt_abort();
  }
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

double MC_process_clock_get(smx_process_t process)
{
  if(mc_time)
    return mc_time[process->pid];
  else
    return 0;
}

void MC_diff(void){

  mc_snapshot_t sn = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot_liveness(sn);

  int i;

  XBT_INFO("Number of regions : %u", sn->num_reg);

  for(i=0; i<sn->num_reg; i++){
    
    switch(sn->regions[i]->type){
    case 0: /* heap */
      XBT_INFO("Size of heap : %zu", sn->regions[i]->size);
      break;
    case 1 : /* libsimgrid */
      XBT_INFO("Size of libsimgrid : %zu", sn->regions[i]->size);
      break;
    case 2 : /* data program */
      XBT_INFO("Size of data program : %zu", sn->regions[i]->size);
      break;
    case 3 : /* stack */
      XBT_INFO("Size of stack : %zu", sn->regions[i]->size);
      XBT_INFO("Start addr of stack : %p", sn->regions[i]->start_addr);
      break;
    }

  }

}

void MC_automaton_load(const char *file){

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();
  
  xbt_automaton_load(_mc_property_automaton,file);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;

}

void MC_automaton_new_propositional_symbol(const char* id, void* fct) {

  raw_mem_set = (mmalloc_get_current_heap() == raw_heap);

  MC_SET_RAW_MEM;

  if (_mc_property_automaton == NULL)
    _mc_property_automaton = xbt_automaton_new();

  xbt_new_propositional_symbol(_mc_property_automaton,id,fct);

  MC_UNSET_RAW_MEM;

  if(raw_mem_set)
    MC_SET_RAW_MEM;
  else
    MC_UNSET_RAW_MEM;
  
}
