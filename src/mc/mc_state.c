#include "../simix/private.h"
#include "xbt/fifo.h"
#include "private.h"

static void MC_state_add_transition(mc_state_t state, mc_transition_t trans);

/**
 * \brief Creates a state data structure used by the exploration algorithm
 */
mc_state_t MC_state_new(void)
{
  mc_state_t state = NULL;
  
  state = xbt_new0(s_mc_state_t, 1);
  state->interleave = xbt_setset_new_set(mc_setset);
  state->done = xbt_setset_new_set(mc_setset);
  
  mc_stats->expanded_states++;
  return state;
}

/**
 * \brief Deletes a state data structure
 * \param trans The state to be deleted
 */
void MC_state_delete(mc_state_t state)
{
  xbt_setset_destroy_set(state->interleave);
  xbt_setset_destroy_set(state->done);
  xbt_free(state);
}

void MC_state_set_executed_request(mc_state_t state, smx_req_t req)
{
  state->executed = *req;
}

smx_req_t MC_state_get_executed_request(mc_state_t state)
{
  return &state->executed;
}

smx_req_t MC_state_get_request(mc_state_t state)
{
  smx_process_t process = NULL;  
  while((process = xbt_setset_set_extract(state->interleave))){
    if(SIMIX_process_is_enabled(process)
       && !xbt_setset_set_belongs(state->done, process)){
      xbt_setset_set_insert(state->done, process);
      return process->request;
    }
  }
   
  return NULL;
}