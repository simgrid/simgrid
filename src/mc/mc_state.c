

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

mc_state_t MC_state_pair_new(void)
{
  mc_state_t state = NULL;
  
  state = xbt_new0(s_mc_state_t, 1);
  state->max_pid = simix_process_maxpid;
  state->proc_status = xbt_new0(s_mc_procstate_t, state->max_pid);
  
  //mc_stats->expanded_states++;
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

void MC_state_set_executed_request(mc_state_t state, smx_req_t req, int value)
{
  state->executed_req = *req;
  state->req_num = value;

  /* The waitany and testany request are transformed into a wait or test request over the
   * corresponding communication action so it can be treated later by the dependence
   * function. */
  switch(req->call){
    case REQ_COMM_WAITANY:
      state->internal_req.call = REQ_COMM_WAIT;
      state->internal_req.issuer = req->issuer;
      state->internal_comm = *xbt_dynar_get_as(req->comm_waitany.comms, value, smx_action_t);
      state->internal_req.comm_wait.comm = &state->internal_comm;
      state->internal_req.comm_wait.timeout = 0;
      break;

    case REQ_COMM_TESTANY:
      state->internal_req.call = REQ_COMM_TEST;
      state->internal_req.issuer = req->issuer;

      if(value > 0)
        state->internal_comm = *xbt_dynar_get_as(req->comm_testany.comms, value, smx_action_t);

      state->internal_req.comm_wait.comm = &state->internal_comm;
      state->internal_req.comm_test.result = value;
      break;

    case REQ_COMM_WAIT:
      state->internal_req = *req;
      state->internal_comm = *(req->comm_wait.comm);
      state->executed_req.comm_wait.comm = &state->internal_comm;
      state->internal_req.comm_wait.comm = &state->internal_comm;
      break;

    case REQ_COMM_TEST:
      state->internal_req = *req;
      state->internal_comm = *req->comm_test.comm;
      state->executed_req.comm_test.comm = &state->internal_comm;
      state->internal_req.comm_test.comm = &state->internal_comm;
      break;

    default:
      state->internal_req = *req;
      break;
  }
}

smx_req_t MC_state_get_executed_request(mc_state_t state, int *value)
{
  *value = state->req_num;
  return &state->executed_req;
}

smx_req_t MC_state_get_internal_request(mc_state_t state)
{
  return &state->internal_req;
}

smx_req_t MC_state_get_request(mc_state_t state, int *value)
{
  smx_process_t process = NULL;
  mc_procstate_t procstate = NULL;
  unsigned int start_count;

  xbt_swag_foreach(process, simix_global->process_list){
    procstate = &state->proc_status[process->pid];

    if(procstate->state == MC_INTERLEAVE){
      if(MC_process_is_enabled(process)){
        switch(process->request.call){
          case REQ_COMM_WAITANY:
            *value = -1;
            while(procstate->interleave_count < xbt_dynar_length(process->request.comm_waitany.comms)){
              if(MC_request_is_enabled_by_idx(&process->request, procstate->interleave_count++)){
                *value = procstate->interleave_count-1;
                break;
              }
            }

            if(procstate->interleave_count >= xbt_dynar_length(process->request.comm_waitany.comms))
              procstate->state = MC_DONE;

            if(*value != -1)
              return &process->request;

            break;

          case REQ_COMM_TESTANY:
            start_count = procstate->interleave_count;
            *value = -1;
            while(procstate->interleave_count < xbt_dynar_length(process->request.comm_testany.comms)){
              if(MC_request_is_enabled_by_idx(&process->request, procstate->interleave_count++)){
                *value = procstate->interleave_count - 1;
                break;
              }
            }

            if(procstate->interleave_count >= xbt_dynar_length(process->request.comm_testany.comms))
              procstate->state = MC_DONE;

            if(*value != -1 || start_count == 0)
              return &process->request;

            break;

          case REQ_COMM_WAIT:
            if(process->request.comm_wait.comm->comm.src_proc
               && process->request.comm_wait.comm->comm.dst_proc){
              *value = 0;
            }else{
              *value = -1;
            }
            procstate->state = MC_DONE;
            return &process->request;

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
