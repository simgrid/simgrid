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
  state->proc_status = xbt_new0(s_mc_procstate_t, state->max_pid);
  
  mc_stats->expanded_states++;
  return state;
}

/**
 * \brief Deletes a state data structure
 * \param trans The state to be deleted
 */
void MC_state_delete(mc_state_t state)
{
  xbt_free(state->proc_status);
  xbt_free(state);
}

void MC_state_add_to_interleave(mc_state_t state, smx_process_t process)
{
  state->proc_status[process->pid].state = MC_INTERLEAVE;
  state->proc_status[process->pid].num_to_interleave = 1;
}

unsigned int MC_state_interleave_size(mc_state_t state)
{
  unsigned int i, size=0;

  for(i=0; i < state->max_pid; i++){
    if(state->proc_status[i].state == MC_INTERLEAVE)
      size++;
  }

  return size;
}

int MC_state_process_is_done(mc_state_t state, smx_process_t process){
  return state->proc_status[process->pid].state == MC_DONE ? TRUE : FALSE;
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
  mc_procstate_t procstate = NULL;

  for(i=0; i < state->max_pid; i++){
    procstate = &state->proc_status[i];

    if(procstate->state == MC_INTERLEAVE){

      if(procstate->num_to_interleave-- > 1)
        *value = procstate->requests_indexes[procstate->num_to_interleave];

      /* If the are no more requests to interleave for process i then it is done */
      if(procstate->num_to_interleave == 0)
        procstate->state = MC_DONE;

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
