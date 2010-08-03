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
  mc_transition_t trans, trans_tmp = NULL;
  xbt_setset_cursor_t cursor = NULL;
  
  /* Create the initial state and push it into the exploration stack */
  MC_SET_RAW_MEM;
  initial_state = MC_state_new();
  xbt_fifo_unshift(mc_stack, initial_state);
  MC_UNSET_RAW_MEM;

  /* Schedule all the processes to detect the transitions of the initial state */
  DEBUG0("**************************************************"); 
  DEBUG0("Initial state");

  while(xbt_swag_size(simix_global->process_to_run)){
    MC_schedule_enabled_processes();
    MC_execute_surf_actions();
  }

  MC_SET_RAW_MEM;
  MC_trans_compute_enabled(initial_state->enabled_transitions,
                           initial_state->transitions);

  /* Fill the interleave set of the initial state with an enabled process */
  trans = xbt_setset_set_choose(initial_state->enabled_transitions);  
  if(trans){
    DEBUG1("Choosing process %s for next interleave", trans->process->name);
    xbt_setset_foreach(initial_state->enabled_transitions, cursor, trans_tmp){
      if(trans_tmp->process == trans->process)
        xbt_setset_set_insert(initial_state->interleave, trans_tmp);    
    }
  }
  MC_UNSET_RAW_MEM;

  /* Update Statistics */
  mc_stats->state_size += xbt_setset_set_size(initial_state->enabled_transitions);
}


/**
 * 	\brief Perform the model-checking operation using a depth-first search exploration
 *         with Dynamic Partial Order Reductions
 */
void MC_dpor(void)
{
  mc_transition_t trans, trans_tmp = NULL;  
  mc_state_t next_state = NULL;
  xbt_setset_cursor_t cursor = NULL;
  
  while(xbt_fifo_size(mc_stack) > 0){
  
    DEBUG0("**************************************************");

    /* FIXME: Think about what happen if there are no transitions but there are
       some actions on the models. (ex. all the processes do a sleep(0) in a round). */

    /* Get current state */
    mc_current_state = (mc_state_t) xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
   
    /* If there are transitions to execute and the maximun depth has not been reached 
       then perform one step of the exploration algorithm */
    if(xbt_setset_set_size(mc_current_state->interleave) > 0 && xbt_fifo_size(mc_stack) < MAX_DEPTH){

      DEBUG4("Exploration detph=%d (state=%p)(%d interleave) (%d enabled)", xbt_fifo_size(mc_stack),
        mc_current_state, xbt_setset_set_size(mc_current_state->interleave),
        xbt_setset_set_size(mc_current_state->enabled_transitions));
      
      /* Update statistics */
      mc_stats->visited_states++;
      mc_stats->executed_transitions++;
      
      /* Choose a transition to execute from the interleave set of the current
         state, and create the data structures for the new expanded state in the
         exploration stack. */
      MC_SET_RAW_MEM;
      trans = xbt_setset_set_extract(mc_current_state->interleave);

      /* Add the transition in the done set of the current state */
      xbt_setset_set_insert(mc_current_state->done, trans);
      
      next_state = MC_state_new();
      xbt_fifo_unshift(mc_stack, next_state);
      
      /* Set it as the executed transition of the current state */
      mc_current_state->executed_transition = trans;
      MC_UNSET_RAW_MEM;

      /* Execute the selected transition by scheduling it's associated process.
         Then schedule every process that got ready to run due to the execution
         of the transition */
      DEBUG1("Executing transition %s", trans->name);
      SIMIX_process_schedule(trans->process);
      MC_execute_surf_actions();
      
      while(xbt_swag_size(simix_global->process_to_run)){
        MC_schedule_enabled_processes();
        MC_execute_surf_actions();
      }
      
      /* Calculate the enabled transitions set of the next state */
      MC_SET_RAW_MEM;

      xbt_setset_foreach(mc_current_state->transitions, cursor, trans_tmp){
        if(trans_tmp->process != trans->process){
          xbt_setset_set_insert(next_state->transitions, trans_tmp);
        }
      }
      
      MC_trans_compute_enabled(next_state->enabled_transitions, next_state->transitions);
      
      /* Choose one transition to interleave from the enabled transition set */      
      trans = xbt_setset_set_choose(next_state->enabled_transitions);
      if(trans){
        DEBUG1("Choosing process %s for next interleave", trans->process->name);
        xbt_setset_foreach(next_state->enabled_transitions, cursor, trans_tmp){
          if(trans_tmp->process == trans->process)
            xbt_setset_set_insert(next_state->interleave, trans_tmp);    
        }
      }
      MC_UNSET_RAW_MEM;

      /* Update Statistics */
      mc_stats->state_size += xbt_setset_set_size(next_state->enabled_transitions);

      /* Let's loop again */
      
    /* The interleave set is empty or the maximum depth is reached, let's back-track */
    }else{
      DEBUG0("There are no more transitions to interleave.");

  
      /* Check for deadlocks */
      xbt_setset_substract(mc_current_state->transitions, mc_current_state->done);
      if(xbt_setset_set_size(mc_current_state->transitions) > 0){
        INFO0("**************************");
        INFO0("*** DEAD-LOCK DETECTED ***");
        INFO0("**************************");
        INFO0("Locked transitions:");
        xbt_setset_foreach(mc_current_state->transitions, cursor, trans){
          INFO1("%s", trans->name);
        }

        INFO0("Counter-example execution trace:");
        MC_dump_stack(mc_stack);
        
        MC_print_statistics(mc_stats);
        xbt_abort();
      }
              
      mc_transition_t q = NULL;
      xbt_fifo_item_t item = NULL;
      mc_state_t state = NULL;

      /*
      INFO0("*********************************");
      MC_show_stack(mc_stack);*/
      
      /* Trash the current state, no longer needed */
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack);
      MC_state_delete(mc_current_state);
      
      /* Traverse the stack backwards until a state with a non empty interleave
         set is found, deleting all the states that have it empty in the way.
         For each deleted state, check if the transition that has generated it 
         (from it's predecesor state), depends on any other previous transition 
         executed before it. If it does then add it to the interleave set of the
         state that executed that previous transition. */
      while((mc_current_state = xbt_fifo_shift(mc_stack)) != NULL){
        q = mc_current_state->executed_transition;
        xbt_fifo_foreach(mc_stack, item, state, mc_state_t){
          if(MC_transition_depend(q, state->executed_transition)){
            xbt_setset_foreach(state->enabled_transitions, cursor, trans){
              if((trans->process == q->process) && !xbt_setset_set_belongs(state->done, trans)){
                DEBUG3("%s depend with %s at %p", q->name, 
                       state->executed_transition->name, state);

                xbt_setset_foreach(state->enabled_transitions, cursor, trans){
                  if(trans->process == q->process)
                    xbt_setset_set_insert(state->interleave, trans);    
                }
                /* FIXME: hack to make it work*/
                trans = q;
                break;
              }
            }
            if(trans)
              break;
          }
        }
        if(xbt_setset_set_size(mc_current_state->interleave) > 0){
          /* We found a back-tracking point, let's loop */
          xbt_fifo_unshift(mc_stack, mc_current_state);
          DEBUG1("Back-tracking to depth %d", xbt_fifo_size(mc_stack));
          MC_replay(mc_stack);
          MC_UNSET_RAW_MEM;
          break;
        }else{
          MC_state_delete(mc_current_state);
        }        
      }
    }
  }
  DEBUG0("We are done");
  /* We are done, show the statistics and return */
  MC_print_statistics(mc_stats);
}