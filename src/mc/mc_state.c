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
  state->max_pid = simix_process_maxpid;
  state->interleave = xbt_new0(char, state->max_pid);
  
  mc_stats->expanded_states++;
  return state;
}

/**
 * \brief Deletes a state data structure
 * \param trans The state to be deleted
 */
void MC_state_delete(mc_state_t state)
{
  xbt_free(state->interleave);
  xbt_free(state);
}

void MC_state_add_to_interleave(mc_state_t state, smx_process_t process)
{
  state->interleave[process->pid] = 1;
}

unsigned int MC_state_interleave_size(mc_state_t state)
{
  unsigned int i, size=0;

  for(i=0; i < state->max_pid; i++){
    if(state->interleave[i] != 0 && state->interleave[i] != -1)
      size++;
  }

  return size;
}

int MC_state_process_is_done(mc_state_t state, smx_process_t process){
  return state->interleave[process->pid] == -1 ? TRUE : FALSE;
}

void MC_state_set_executed_request(mc_state_t state, smx_req_t req)
{
  state->executed = *req;
}

smx_req_t MC_state_get_executed_request(mc_state_t state)
{
  return &state->executed;
}

smx_req_t MC_state_get_request(mc_state_t state, char *value)
{
  unsigned int i;
  smx_process_t process = NULL;

  for(i=0; i < state->max_pid; i++){
    if(state->interleave[i] > 0){
      *value = state->interleave[i]--;

      /* If 0 was reached means that the process is done, so we
       * set it's value to -1 (the "done" value) */
      if(state->interleave[i] == 0)
        state->interleave[i]--;

      /* FIXME: SIMIX should implement a process table indexed by pid */
      /* So we should use that instead of traversing the swag */
      xbt_swag_foreach(process, simix_global->process_list){
        if(process->pid == i)
          break;
      }

      if(SIMIX_process_is_enabled(process))
        return &process->request;
    }
  }

  return NULL;
}
