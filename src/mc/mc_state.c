#include "../simix/private.h"
#include "xbt/fifo.h"
#include "private.h"

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

void MC_state_interleave_process(mc_state_t state, smx_process_t process)
{
  state->proc_status[process->pid].state = MC_INTERLEAVE;
  state->proc_status[process->pid].interleave_count = 0;
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

void MC_state_set_executed_request(mc_state_t state, smx_req_t req, unsigned int value)
{
  state->executed_value = value;
  state->executed = *req;
}

smx_req_t MC_state_get_executed_request(mc_state_t state, unsigned int *value)
{
  *value = state->executed_value;
  return &state->executed;
}

smx_req_t MC_state_get_request(mc_state_t state, unsigned int *value)
{
  smx_process_t process = NULL;
  mc_procstate_t procstate = NULL;


  xbt_swag_foreach(process, simix_global->process_list){
    procstate = &state->proc_status[process->pid];

    if(procstate->state == MC_INTERLEAVE){
      if(SIMIX_process_is_enabled(process)){
        switch(process->request.call){
          case REQ_COMM_WAITANY:
            while(procstate->interleave_count < xbt_dynar_length(process->request.comm_waitany.comms)){
              if(SIMIX_request_is_enabled_by_idx(&process->request, procstate->interleave_count++)){
                *value = procstate->interleave_count - 1;
                return &process->request;
              }
            }
            procstate->state = MC_DONE;
            break;

          case REQ_COMM_TESTANY:
            if(MC_request_testany_fail(&process->request)){
              procstate->state = MC_DONE;
              *value = (unsigned int)-1;
              return &process->request;
            }

            while(procstate->interleave_count < xbt_dynar_length(process->request.comm_waitany.comms)){
              if(SIMIX_request_is_enabled_by_idx(&process->request, procstate->interleave_count++)){
                *value = procstate->interleave_count - 1;
                return &process->request;
              }
            }
            procstate->state = MC_DONE;
            break;

          default:
            procstate->state = MC_DONE;
            *value = 0;
            return &process->request;
            break;
        }
      }
    }
  }

  return NULL;
}
