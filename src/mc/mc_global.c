/* Copyright (c) 2008-2012 Da SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "../surf/surf_private.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
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
int compare;
xbt_dynar_t mc_binary_local_variables = NULL;

extern xbt_dynar_t mmalloc_ignore;
extern xbt_dynar_t stacks_areas;

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

void MC_compare(void){
  compare = 1;
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

  /* mc_time refers to clock for each process -> ignore it for heap comparison */
  int i;
  for(i = 0; i<simix_process_maxpid; i++)
    MC_ignore(&(mc_time[i]), sizeof(double));
  
  compare = 0;

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */

  MC_SET_RAW_MEM;

  mc_binary_local_variables = xbt_dynar_new(sizeof(dw_frame_t), NULL);

  /* Initialize statistics */
  mc_stats_pair = xbt_new0(s_mc_stats_pair_t, 1);

  XBT_DEBUG("Creating stack");

  /* Create exploration stack */
  mc_stack_liveness = xbt_fifo_new();

  MC_UNSET_RAW_MEM;

  /* Get local variables in binary for state equality detection */
  MC_get_binary_local_variables();

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

/************ MC_ignore ***********/ 

void MC_ignore_init(){
  MC_SET_RAW_MEM;
  mmalloc_ignore = xbt_dynar_new(sizeof(mc_ignore_region_t), NULL);
  stacks_areas = xbt_dynar_new(sizeof(stack_region_t), NULL);
  MC_UNSET_RAW_MEM;
}

void MC_ignore(void *address, size_t size){

  MC_SET_RAW_MEM;

  mc_ignore_region_t region = NULL;
  region = xbt_new0(s_mc_ignore_region_t, 1);
  region->address = address;
  region->size = size;
  region->block = ((char*)address - (char*)((xbt_mheap_t)std_heap)->heapbase) / BLOCKSIZE + 1;

  if(((xbt_mheap_t)std_heap)->heapinfo[region->block].type == 0){
    region->fragment = -1;
  }else{
    region->fragment = ((uintptr_t) (ADDR2UINT (address) % (BLOCKSIZE))) >> ((xbt_mheap_t)std_heap)->heapinfo[region->block].type;
  }

  unsigned int cursor = 0;
  mc_ignore_region_t current_region;
  xbt_dynar_foreach(mmalloc_ignore, cursor, current_region){
    if(current_region->address > address)
      break;
  }

  xbt_dynar_insert_at(mmalloc_ignore, cursor, &region);

  MC_UNSET_RAW_MEM;
}

void MC_new_stack_area(void *stack, char *name){
  MC_SET_RAW_MEM;
  stack_region_t region = NULL;
  region = xbt_new0(s_stack_region_t, 1);
  region->address = stack;
  region->process_name = strdup(name);
  xbt_dynar_push(stacks_areas, &region);
  MC_UNSET_RAW_MEM;
}

/************ DWARF ***********/

static e_dw_location_type get_location(char *expr, dw_location_t entry);

void MC_get_binary_local_variables(){

  char *command = bprintf("dwarfdump -i %s", xbt_binary_name); 
  
  FILE* fp = popen(command, "r");

  if(fp == NULL)
    perror("popen failed");

  char *line = NULL, *tmp_line = NULL, *tmp_location = NULL, *frame_name = NULL;
  ssize_t read;
  size_t n = 0;
  int valid_variable = 1, valid_frame = 1;
  char *node_type = NULL, *location_type = NULL, *variable_name = NULL, *lowpc = NULL, *highpc = NULL;
  xbt_dynar_t split = NULL;

  void *low_pc = NULL, *old_low_pc = NULL;

  int compile_unit_found = 0; /* Detect if the program has been compiled with -g */

  read = getline(&line, &n, fp);

  while (read != -1) {

    if(n == 0)
      continue;

    /* Wipeout the new line character */
    line[read - 1] = '\0';
    
    if(line[0] == '<'){

      /* If the program hasn't been compiled with -g, no symbol (line starting with '<' ) found */
      compile_unit_found = 1;

      /* Get node type */
      strtok(line, " ");
      strtok(NULL, " ");
      node_type = strtok(NULL, " ");

      if(strcmp(node_type, "DW_TAG_subprogram") == 0){ /* New frame */

        read = getline(&line, &n, fp);

        while(read != -1 && line[0] != '<'){

          if(n == 0)
            continue;

          node_type = strtok(line, " ");

          if(node_type != NULL && strcmp(node_type, "DW_AT_name") == 0){

            frame_name = strdup(strtok(NULL, " "));
            xbt_str_trim(frame_name, NULL);
            xbt_str_trim(frame_name, "\"");
            read = getline(&line, &n, fp);

          }else if(node_type != NULL && strcmp(node_type, "DW_AT_frame_base") == 0){

            if(valid_frame == 1){

              dw_frame_t frame = xbt_new0(s_dw_frame_t, 1);
              frame->name = strdup(frame_name);
              frame->variables = xbt_dynar_new(sizeof(dw_local_variable_t), NULL);
              frame->location = xbt_new0(s_dw_location_t, 1);
            
              location_type = strtok(NULL, " ");

              if(strcmp(location_type, "<loclist") == 0){

                frame->location->type = e_dw_loclist;
                frame->location->location.loclist = xbt_dynar_new(sizeof(dw_location_entry_t), NULL);
             
                read = getline(&line, &n, fp);
                xbt_str_ltrim(line, NULL);
                
                while(read != -1 && line[0] == '['){

                  strtok(line, "<");
                  lowpc = strdup(strtok(NULL, "<"));
                  highpc = strdup(strtok(NULL, ">"));
                  tmp_location = strdup(strtok(NULL, ">"));
                  lowpc[strlen(lowpc) - 1] = '\0'; /* Remove last character '>' */
             
                  dw_location_entry_t new_entry = xbt_new0(s_dw_location_entry_t, 1);
                
                  if(low_pc == NULL){
                    strtok(lowpc, "=");
                    new_entry->lowpc = (void *) strtoul(strtok(NULL, "="), NULL, 16);
                    strtok(highpc, "=");
                    new_entry->highpc = (void *) strtoul(strtok(NULL, "="), NULL, 16);             
                  }else{
                    strtok(lowpc, "=");
                    old_low_pc = (void *)strtoul(strtok(NULL, "="), NULL, 16);
                    new_entry->lowpc = (char *)low_pc + (long)old_low_pc;
                    strtok(highpc, "=");
                    new_entry->highpc = (char*)low_pc + ((char *)((void *)strtoul(strtok(NULL, "="), NULL, 16)) - (char*)old_low_pc);
                  }

                  new_entry->location = xbt_new0(s_dw_location_t, 1);
                
                  get_location(tmp_location, new_entry->location);
                
                  xbt_dynar_push(frame->location->location.loclist, &new_entry);

                  read = getline(&line, &n, fp);
                  xbt_str_ltrim(line, NULL);
                }

              }else{
                read = getline(&line, &n, fp);
                frame->location->type = get_location(location_type, frame->location);

              }

              xbt_dynar_push(mc_binary_local_variables, &frame);

            }else{

               read = getline(&line, &n, fp);

            }
 
          }else if(node_type != NULL && (strcmp(node_type, "DW_AT_declaration") == 0 || strcmp(node_type, "DW_AT_abstract_origin") == 0 || strcmp(node_type, "DW_AT_artificial") == 0)){

            read = getline(&line, &n, fp);
            valid_frame = 0;
          
          }else{

            read = getline(&line, &n, fp);

          }         

        }
        
        valid_frame = 1;

      }else if(strcmp(node_type, "DW_TAG_variable") == 0){ /* New variable */
        
        variable_name = NULL;
        location_type = NULL;
        
        read = getline(&line, &n, fp);

        while(read != -1 && line[0] != '<'){

          if(n == 0)
            continue;

          tmp_line = strdup(line);
          
          node_type = strtok(line, " ");

          if(node_type != NULL && strcmp(node_type, "DW_AT_name") == 0){

            variable_name = strdup(strtok(NULL, " "));
            xbt_str_trim(variable_name, NULL);
            xbt_str_trim(variable_name, "\"");
            read = getline(&line, &n, fp);
            
          }else if(node_type != NULL && strcmp(node_type, "DW_AT_location") == 0){

            if(valid_variable == 1){

              location_type = strdup(strtok(NULL, " "));

              dw_local_variable_t variable = xbt_new0(s_dw_local_variable_t, 1);
              variable->name = strdup(variable_name);
              variable->location = xbt_new0(s_dw_location_t, 1);
            
              if(strcmp(location_type, "<loclist") == 0){

                variable->location->type = e_dw_loclist;
                variable->location->location.loclist = xbt_dynar_new(sizeof(dw_location_entry_t), NULL);

                read = getline(&line, &n, fp);
                xbt_str_ltrim(line, NULL);
                
                while(read != -1 && line[0] == '['){

                  strtok(line, "<");
                  lowpc = strdup(strtok(NULL, "<"));
                  highpc = strdup(strtok(NULL, ">"));
                  tmp_location = strdup(strtok(NULL, ">"));
                  lowpc[strlen(lowpc) - 1] = '\0'; /* Remove last character '>' */

                  dw_location_entry_t new_entry = xbt_new0(s_dw_location_entry_t, 1);

                  if(low_pc == NULL){
                    strtok(lowpc, "=");
                    new_entry->lowpc = (void *) strtoul(strtok(NULL, "="), NULL, 16);
                    strtok(highpc, "=");
                    new_entry->highpc = (void *) strtoul(strtok(NULL, "="), NULL, 16);             
                  }else{
                    strtok(lowpc, "=");
                    old_low_pc = (void *)strtoul(strtok(NULL, "="), NULL, 16);
                    new_entry->lowpc = (char *)low_pc + (long)old_low_pc;
                    strtok(highpc, "=");
                    new_entry->highpc = (char*)low_pc + ((char *)((void *)strtoul(strtok(NULL, "="), NULL, 16)) - (char*)old_low_pc);
                  }

                  new_entry->location = xbt_new0(s_dw_location_t, 1);
                
                  get_location(tmp_location, new_entry->location);
                  
                  xbt_dynar_push(variable->location->location.loclist, &new_entry);

                  read = getline(&line, &n, fp);
                  xbt_str_ltrim(line, NULL);
                }
              
              }else{
                
                xbt_str_strip_spaces(tmp_line);
                split = xbt_str_split(tmp_line, " ");
                xbt_dynar_remove_at(split, 0, NULL);
                location_type = xbt_str_join(split, " ");
                
                variable->location->type = get_location(location_type, variable->location);
                read = getline(&line, &n, fp);

              }

              xbt_dynar_push(((dw_frame_t)xbt_dynar_get_as(mc_binary_local_variables, xbt_dynar_length(mc_binary_local_variables) - 1, dw_frame_t))->variables, &variable);

            }else{

              read = getline(&line, &n, fp);

            }
           
          }else if(node_type != NULL && (strcmp(node_type, "DW_AT_artificial") == 0 || strcmp(node_type, "DW_AT_external") == 0)){

            valid_variable = 0;
            read = getline(&line, &n, fp);

          }else{

            read = getline(&line, &n, fp);

          }
      
        }

        valid_variable = 1;

      }else if(strcmp(node_type, "DW_TAG_compile_unit") == 0){

        read = getline(&line, &n, fp);
        
        while(read != -1 && line[0] != '<'){
          
          if(n == 0)
            continue;
          
          node_type = strtok(line, " ");
          
          if(node_type != NULL && strcmp(node_type, "DW_AT_low_pc") == 0){
            low_pc = (void *) strtoul(strtok(NULL, " "), NULL, 16);
          }

          read = getline(&line, &n, fp);

        }

      }else{

        read = getline(&line, &n, fp);

      }
 
    }else{

      read = getline(&line, &n, fp);

    }

  }

  if(compile_unit_found == 0){
    XBT_INFO("Your program must be compiled with -g");
    xbt_abort();
  }

  if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug))
    print_local_variables(mc_binary_local_variables);

  free(line); free(tmp_line); free(tmp_location); free(frame_name);
  free(node_type); free(location_type); free(variable_name); free(lowpc); free(highpc);
  free(command);
  pclose(fp);
}

void print_local_variables(xbt_dynar_t list){
  
  dw_frame_t frame;
  dw_local_variable_t variable;
  dw_location_entry_t entry;
  dw_location_t location_entry;
  unsigned int cursor = 0, cursor2 = 0, cursor3 = 0, cursor4 = 0;

  xbt_dynar_foreach(list, cursor, frame){
    fprintf(stderr, "Frame name : %s", frame->name);
    fprintf(stderr, "Location type : %d\n", frame->location->type); 
    fprintf(stderr, "Variables : (%lu)\n", xbt_dynar_length(frame->variables));
    xbt_dynar_foreach(frame->variables, cursor2, variable){
      fprintf(stderr, "Name : %s", variable->name);
      fprintf(stderr, "Location type : %d\n", variable->location->type);
      switch(variable->location->type){
      case e_dw_loclist :
        xbt_dynar_foreach(variable->location->location.loclist, cursor3, entry){
          fprintf(stderr, "Lowpc : %p, Highpc : %p,", entry->lowpc, entry->highpc);
          switch(entry->location->type){
          case e_dw_register :
            fprintf(stderr, " Location : in register %d\n", entry->location->location.reg);
            break;
          case e_dw_bregister_op:
            fprintf(stderr, " Location : Add %d to the value in register %d\n", entry->location->location.breg_op.offset, entry->location->location.breg_op.reg);
            break;
          case e_dw_lit:
            fprintf(stderr, "Value already kwnown : %d\n", entry->location->location.lit);
            break;
          case e_dw_fbregister_op:
            fprintf(stderr, " Location : %d bytes from logical frame pointer\n", entry->location->location.fbreg_op);
            break;
          case e_dw_compose:
            fprintf(stderr, " Location :\n");
            xbt_dynar_foreach(entry->location->location.compose, cursor4, location_entry){
              switch(location_entry->type){
              case e_dw_register :
                fprintf(stderr, " %d) in register %d\n", cursor4 + 1, location_entry->location.reg);
                break;
              case e_dw_bregister_op:
                fprintf(stderr, " %d) add %d to the value in register %d\n", cursor4 + 1, location_entry->location.breg_op.offset, location_entry->location.breg_op.reg);
                break;
              case e_dw_lit:
                fprintf(stderr, "%d) Value already kwnown : %d\n", cursor4 + 1, location_entry->location.lit);
                break;
              case e_dw_fbregister_op:
                fprintf(stderr, " %d) %d bytes from logical frame pointer\n", cursor4 + 1, location_entry->location.fbreg_op);
                break;
              case e_dw_deref:
                fprintf(stderr, " %d) Pop the stack entry and treats it as an address (size of data %d)\n", cursor4 + 1, location_entry->location.deref_size);      
                break;
              case e_dw_arithmetic : 
                fprintf(stderr, "%d) arithmetic operation : %s\n", cursor4 + 1, location_entry->location.arithmetic);
                break;
              case e_dw_piece:
                fprintf(stderr, "%d) The %d byte(s) previous value\n", cursor4 + 1, location_entry->location.piece);
                break;
              case e_dw_constant :
                fprintf(stderr, "%d) Constant %d\n", cursor4 + 1, location_entry->location.constant.value);
                break;
              default : 
                fprintf(stderr, "%d) Location type not supported\n", cursor4 + 1);
                break;
              }
            }
            break;
          default:
            fprintf(stderr, "Location type not supported\n");
            break;
          }
        }
        break;
      case e_dw_compose:
        cursor4 = 0;
        fprintf(stderr, "Location :\n");
        xbt_dynar_foreach(variable->location->location.compose, cursor4, location_entry){
          switch(location_entry->type){
          case e_dw_register :
             fprintf(stderr, " %d) in register %d\n", cursor4 + 1, location_entry->location.reg);
            break;
          case e_dw_bregister_op:
            fprintf(stderr, " %d) add %d to the value in register %d\n", cursor4 + 1, location_entry->location.breg_op.offset, location_entry->location.breg_op.reg);
            break;
          case e_dw_lit:
            fprintf(stderr, "%d) Value already kwnown : %d\n", cursor4 + 1, location_entry->location.lit);
            break;
          case e_dw_fbregister_op:
            fprintf(stderr, " %d) %d bytes from logical frame pointer\n", cursor4 + 1, location_entry->location.fbreg_op);
            break;
          case e_dw_deref:
            fprintf(stderr, " %d) Pop the stack entry and treats it as an address (size of data %d)\n", cursor4 + 1, location_entry->location.deref_size);      
            break;
          case e_dw_arithmetic : 
            fprintf(stderr, "%d) arithmetic operation : %s\n", cursor4 + 1, location_entry->location.arithmetic);
            break;
          case e_dw_piece:
            fprintf(stderr, "%d) The %d byte(s) previous value\n", cursor4 + 1, location_entry->location.piece);
            break;
          case e_dw_constant :
            fprintf(stderr, "%d) Constant %d\n", cursor4 + 1, location_entry->location.constant.value);
            break;
          default : 
            fprintf(stderr, "%d) Location type not supported\n", cursor4 + 1);
            break;
          }
        }
        break;
      default :
        fprintf(stderr, "Location type not supported\n");
        break;
      }
    }
  }

}

static e_dw_location_type get_location(char *expr, dw_location_t entry){

  int cursor = 0;
  char *tok = NULL, *tmp_tok = NULL; 

  xbt_dynar_t tokens = xbt_str_split(expr, NULL);
  xbt_dynar_remove_at(tokens, xbt_dynar_length(tokens) - 1, NULL);

  if(xbt_dynar_length(tokens) > 1){

    entry->type = e_dw_compose;
    entry->location.compose = xbt_dynar_new(sizeof(dw_location_t), NULL);

    while(cursor < xbt_dynar_length(tokens)){

      tok = xbt_dynar_get_as(tokens, cursor, char*);
      
      if(strncmp(tok, "DW_OP_reg", 9) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_register;
        if(tok[9] == 'x'){
          new_element->location.reg = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
        }else{
          new_element->location.reg = atoi(strtok(tok, "DW_OP_reg"));
        }
        xbt_dynar_push(entry->location.compose, &new_element);     
      }else if(strcmp(tok, "DW_OP_fbreg") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_fbregister_op;
        new_element->location.fbreg_op = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strncmp(tok, "DW_OP_breg", 10) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_bregister_op;
        if(tok[10] == 'x'){
          new_element->location.breg_op.reg = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
          new_element->location.breg_op.offset = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
        }else{
          if(strchr(tok,'+') != NULL){
            tmp_tok = strtok(tok,"DW_OP_breg"); 
            new_element->location.breg_op.reg = atoi(strtok(tmp_tok,"+"));
            new_element->location.breg_op.offset = atoi(strtok(NULL,"+"));
          }else{
            new_element->location.breg_op.reg = atoi(strtok(tok, "DW_OP_breg"));
            new_element->location.breg_op.offset = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
          }
        }
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strncmp(tok, "DW_OP_lit", 9) == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_lit;
        new_element->location.lit = atoi(strtok(tok, "DW_OP_lit"));
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_piece") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_piece;
        if(strlen(xbt_dynar_get_as(tokens, ++cursor, char*)) > 1)
          new_element->location.piece = atoi(xbt_dynar_get_as(tokens, cursor, char*));
        else
          new_element->location.piece = xbt_dynar_get_as(tokens, cursor, char*)[0] - '0';
        xbt_dynar_push(entry->location.compose, &new_element);

      }else if(strcmp(tok, "DW_OP_abs") == 0 || 
               strcmp(tok, "DW_OP_and") == 0 ||
               strcmp(tok, "DW_OP_div") == 0 ||
               strcmp(tok, "DW_OP_minus") == 0 ||
               strcmp(tok, "DW_OP_mod") == 0 ||
               strcmp(tok, "DW_OP_mul") == 0 ||
               strcmp(tok, "DW_OP_neg") == 0 ||
               strcmp(tok, "DW_OP_not") == 0 ||
               strcmp(tok, "DW_OP_or") == 0 ||
               strcmp(tok, "DW_OP_plus") == 0 ||
               strcmp(tok, "DW_OP_plus_uconst") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_arithmetic;
        new_element->location.arithmetic = strdup(strtok(tok, "DW_OP_"));
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_stack_value") == 0){
        cursor++;
      }else if(strcmp(tok, "DW_OP_deref_size") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_deref;
        if(strlen(xbt_dynar_get_as(tokens, ++cursor, char*)) > 1)
          new_element->location.deref_size = atoi(xbt_dynar_get_as(tokens, cursor, char*));
        else
          new_element->location.deref_size = xbt_dynar_get_as(tokens, cursor, char*)[0] - '0';
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_deref") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_deref;
        new_element->location.deref_size = sizeof(void *);
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_constu") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_constant;
        new_element->location.constant.is_signed = 0;
        new_element->location.constant.bytes = 1;
        if(strlen(xbt_dynar_get_as(tokens, ++cursor, char*)) > 1)
          new_element->location.constant.value = atoi(xbt_dynar_get_as(tokens, cursor, char*));
        else
          new_element->location.constant.value = xbt_dynar_get_as(tokens, cursor, char*)[0] - '0';
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_consts") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_constant;
        new_element->location.constant.is_signed = 1;
        new_element->location.constant.bytes = 1;
        new_element->location.constant.value = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_const1u") == 0 ||
               strcmp(tok, "DW_OP_const2u") == 0 ||
               strcmp(tok, "DW_OP_const4u") == 0 ||
               strcmp(tok, "DW_OP_const8u") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_constant;
        new_element->location.constant.is_signed = 0;
        new_element->location.constant.bytes = tok[11] - '0';
        if(strlen(xbt_dynar_get_as(tokens, ++cursor, char*)) > 1)
          new_element->location.constant.value = atoi(xbt_dynar_get_as(tokens, cursor, char*));
        else
          new_element->location.constant.value = xbt_dynar_get_as(tokens, cursor, char*)[0] - '0';
        xbt_dynar_push(entry->location.compose, &new_element);
      }else if(strcmp(tok, "DW_OP_const1s") == 0 ||
               strcmp(tok, "DW_OP_const2s") == 0 ||
               strcmp(tok, "DW_OP_const4s") == 0 ||
               strcmp(tok, "DW_OP_const8s") == 0){
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_constant;
        new_element->location.constant.is_signed = 1;
        new_element->location.constant.bytes = tok[11] - '0';
        new_element->location.constant.value = atoi(xbt_dynar_get_as(tokens, ++cursor, char*));
        xbt_dynar_push(entry->location.compose, &new_element);
      }else{
        dw_location_t new_element = xbt_new0(s_dw_location_t, 1);
        new_element->type = e_dw_unsupported;
        xbt_dynar_push(entry->location.compose, &new_element);
      }

      cursor++;
      
    }

    free(tok);
    free(tmp_tok);
    /*xbt_dynar_free(&tokens);*/

    return e_dw_compose;

  }else{

    if(strncmp(expr, "DW_OP_reg", 9) == 0){
      entry->type = e_dw_register;
      entry->location.reg = atoi(strtok(expr,"DW_OP_reg"));
    }else if(strncmp(expr, "DW_OP_breg", 10) == 0){
      entry->type = e_dw_bregister_op;
      tok = strtok(expr, "+");
      entry->location.breg_op.offset = atoi(strtok(NULL, "+"));
      entry->location.breg_op.reg = atoi(strtok(tok, "DW_OP_breg"));
    }else{
      entry->type = e_dw_unsupported;
    }

    free(tok);
    free(tmp_tok);

    return entry->type;

  }

}
