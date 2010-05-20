#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_dfs, mc,
				"Logging specific to MC DFS exploration");


void MC_dfs_init()
{
  mc_transition_t trans = NULL;  
  mc_state_t initial_state = NULL;
  xbt_setset_cursor_t cursor = NULL;
  
  MC_SET_RAW_MEM;
  initial_state = MC_state_new();
  /* Add the state data structure for the initial state */
  xbt_fifo_unshift(mc_stack, initial_state);
  MC_UNSET_RAW_MEM;
  
  DEBUG0("**************************************************"); 
  DEBUG0("Initial state");
  
  /* Schedule all the processes to detect the transitions from the initial state */
  MC_schedule_enabled_processes();

  MC_SET_RAW_MEM;
  xbt_setset_add(initial_state->enabled_transitions, initial_state->transitions);
  xbt_setset_foreach(initial_state->enabled_transitions, cursor, trans){
    if(trans->type == mc_wait
       && (trans->wait.comm->src_proc == NULL || trans->wait.comm->dst_proc == NULL)){
      xbt_setset_set_remove(initial_state->enabled_transitions, trans);
    }
  }

  /* Fill the interleave set of the initial state with all enabled transitions */
  xbt_setset_add(initial_state->interleave, initial_state->enabled_transitions);

  /* Update Statistics */
  mc_stats->state_size += xbt_setset_set_size(initial_state->enabled_transitions);
  
  MC_UNSET_RAW_MEM;
}

/**
 * 	\brief Perform the model-checking operation using a depth-first search exploration
 *
 *	It performs the model-checking operation by executing all possible scheduling of the communication actions
 * 	\return The time spent to execute the simulation or -1 if the simulation ended
 */
void MC_dfs(void)
{
  mc_transition_t trans = NULL;  
  mc_state_t current_state = NULL;
  mc_state_t next_state = NULL;
  xbt_setset_cursor_t cursor = NULL;
  
  while(xbt_fifo_size(mc_stack) > 0){ 
    DEBUG0("**************************************************");

    /* FIXME: Think about what happens if there are no transitions but there are
       some actions on the models. (ex. all the processes do a sleep(0) in a round). */

    /* Get current state */
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
   
    /* If there are transitions to execute and the maximun depth has not been reached 
       then perform one step of the exploration algorithm */
    if(xbt_setset_set_size(current_state->interleave) > 0 && xbt_fifo_size(mc_stack) < MAX_DEPTH){

      DEBUG3("Exploration detph=%d (state=%p)(%d transitions)", xbt_fifo_size(mc_stack),
        current_state, xbt_setset_set_size(current_state->interleave));

      /* Update statistics */
      mc_stats->visited_states++;
      mc_stats->executed_transitions++;
      
      /* Choose a transition to execute from the interleave set of the current state,
         and create the data structures for the new expanded state in the exploration
         stack. */
      MC_SET_RAW_MEM;
      trans = xbt_setset_set_extract(current_state->interleave);

      /* Define it as the executed transition of this state */
      current_state->executed_transition = trans;

      next_state = MC_state_new();
      xbt_fifo_unshift(mc_stack, next_state);
      MC_UNSET_RAW_MEM;

      DEBUG1("Executing transition %s", trans->name);
      SIMIX_process_schedule(trans->process);

      /* Do all surf's related black magic tricks to keep all working */
      MC_execute_surf_actions();

      /* Schedule every process that got enabled due to the executed transition */
      MC_schedule_enabled_processes();
      
      /* Calculate the enabled transitions set of the next state:
          -add the transition sets of the current state and the next state 
          -remove the executed transition from that set
          -remove all the transitions that are disabled (mc_wait only)
          -use the resulting set as the enabled transitions of the next state */
      MC_SET_RAW_MEM;
      xbt_setset_add(next_state->transitions, current_state->transitions);
      xbt_setset_set_remove(next_state->transitions, trans);
      xbt_setset_add(next_state->enabled_transitions, next_state->transitions);
      xbt_setset_foreach(next_state->enabled_transitions, cursor, trans){
        if(trans->type == mc_wait
           && (trans->wait.comm->src_proc == NULL || trans->wait.comm->dst_proc == NULL)){
          xbt_setset_set_remove(next_state->enabled_transitions, trans);
        }
      }
        
      /* Fill the interleave set of the new state with all enabled transitions */
      xbt_setset_add(next_state->interleave, next_state->enabled_transitions);
      MC_UNSET_RAW_MEM;

      /* Update Statistics */
      mc_stats->state_size += xbt_setset_set_size(next_state->enabled_transitions);

      /* Let's loop again */
      
    /* The interleave set is empty or the maximum depth is reached, let's back-track */
    }else{    
      DEBUG0("There are no more actions to run");
      
      MC_SET_RAW_MEM;
      xbt_fifo_shift(mc_stack);
      MC_state_delete(current_state);  

      /* Go backwards in the stack until we find a state with transitions in the interleave set */
      while(xbt_fifo_size(mc_stack) > 0 && (current_state = (mc_state_t)xbt_fifo_shift(mc_stack))){
        if(xbt_setset_set_size(current_state->interleave) == 0){
          MC_state_delete(current_state);
        }else{
          xbt_fifo_unshift(mc_stack, current_state);
          DEBUG1("Back-tracking to depth %d", xbt_fifo_size(mc_stack));
          MC_replay(mc_stack);
          MC_UNSET_RAW_MEM;
          /* Let's loop again */
          break;
        }
      }
    }
  }
  MC_UNSET_RAW_MEM;
  /* We are done, show the statistics and return */
  MC_print_statistics(mc_stats);
}