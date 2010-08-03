#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans, mc,
				"Logging specific to MC transitions");

/* Creates a new iSend transition */
mc_transition_t MC_trans_isend_new(smx_rdv_t rdv)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_isend;
  trans->process = SIMIX_process_self();
  trans->isend.rdv = rdv;
  trans->name = bprintf("[%s]\t iSend (%p)", trans->process->smx_host->name, trans); 
  return trans;
}

/* Creates a new iRecv transition */
mc_transition_t MC_trans_irecv_new(smx_rdv_t rdv)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_irecv;
  trans->process = SIMIX_process_self();
  trans->irecv.rdv = rdv;
  trans->name = bprintf("[%s]\t iRecv (%p)", trans->process->smx_host->name, trans);
  return trans;
}

/* Creates a new Wait transition */
mc_transition_t MC_trans_wait_new(smx_comm_t comm)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_wait;
  trans->process = SIMIX_process_self();
  trans->wait.comm = comm;
  trans->name = bprintf("[%s]\t Wait (%p)", trans->process->smx_host->name, trans);
  return trans;
}

/* Creates a new test transition */
mc_transition_t MC_trans_test_new(smx_comm_t comm)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_test;
  trans->process = SIMIX_process_self();
  trans->test.comm = comm;
  trans->name = bprintf("[%s]\t Test (%p)", trans->process->smx_host->name, trans);      
  return trans;
}

/* Creates a new WaitAny transition */
mc_transition_t MC_trans_waitany_new(xbt_dynar_t comms)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_waitany;
  trans->process = SIMIX_process_self();
  trans->waitany.comms = comms;
  trans->name = bprintf("[%s]\t WaitAny (%p)", trans->process->smx_host->name, trans);
  return trans;
}

/* Creates a new Random transition */
mc_transition_t MC_trans_random_new(int value)
{
  mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);

  trans->type = mc_random;
  trans->process = SIMIX_process_self();
  trans->random.value = value;
  trans->name = bprintf("[%s]\t Random %d (%p)", trans->process->smx_host->name, value, trans);

  return trans;
}

/**
 * \brief Deletes a transition
 * \param trans The transition to be deleted
 */
void MC_transition_delete(mc_transition_t trans)
{
  xbt_free(trans->name);
  xbt_free(trans);
}

/************************** Interception Functions ****************************/
/* These functions intercept all the actions relevant to the model-checking
 * process, the only difference between them is the type of transition they
 * create. The interception works by yielding the process performer of the
 * action, and depending on the execution mode it behaves diferently.
 * There are two running modes, a replay (back-track process) in which no 
 * transition needs to be created, or a new state expansion, which in 
 * consecuence a transition needs to be created and added to the set of
 * transitions associated to the current state.
 */

/**
 * \brief Intercept an iSend action
 * \param rdv The rendez-vous of the iSend
 */
void MC_trans_intercept_isend(smx_rdv_t rdv)
{
  mc_transition_t trans=NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    trans = MC_trans_isend_new(rdv);
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->created_transitions, trans);
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();  
}

/**
 * \brief Intercept an iRecv action
 * \param rdv The rendez-vous of the iRecv
 */
void MC_trans_intercept_irecv(smx_rdv_t rdv)
{
  mc_transition_t trans=NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    trans = MC_trans_irecv_new(rdv);
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->created_transitions, trans);
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();
}

/**
 * \brief Intercept a Wait action
 * \param comm The communication associated to the wait
 */
void MC_trans_intercept_wait(smx_comm_t comm)
{
  mc_transition_t trans=NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    trans = MC_trans_wait_new(comm);
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->created_transitions, trans);
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();
}

/**
 * \brief Intercept a Test action
 * \param comm The communication associated to the Test
 */
void MC_trans_intercept_test(smx_comm_t comm)
{
  mc_transition_t trans=NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    trans = MC_trans_test_new(comm);
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->created_transitions, trans);
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();
}

/**
 * \brief Intercept a WaitAny action
 * \param comms The communications associated to the WaitAny
 */
void MC_trans_intercept_waitany(xbt_dynar_t comms)
{
  mc_transition_t trans = NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    trans = MC_trans_waitany_new(comms);
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->created_transitions, trans);
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();
}

/**
 * \brief Intercept a Random action
 * \param min The minimum value that can be returned by the Random action
 * \param max The maximum value that can be returned by the Random action
 */
void MC_trans_intercept_random(int min, int max)
{
  int i;
  mc_transition_t trans=NULL;
  mc_state_t current_state = NULL;
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    for(i=min; i <= max; i++){    
      trans = MC_trans_random_new(i);
      xbt_setset_set_insert(current_state->created_transitions, trans);
      xbt_setset_set_insert(current_state->transitions, trans);
    }
    MC_UNSET_RAW_MEM;
  }
  SIMIX_process_yield();
}

/* Compute the subset of enabled transitions of the transition set */
void MC_trans_compute_enabled(xbt_setset_set_t enabled, xbt_setset_set_t transitions)
{
  unsigned int index = 0;
  smx_comm_t comm = NULL;
  mc_transition_t trans = NULL;
  xbt_setset_cursor_t cursor = NULL;
  xbt_setset_foreach(transitions, cursor, trans){
    switch(trans->type){
      /* Wait transitions are enabled only if the communication has both a
         sender and receiver */
      case mc_wait:
        if(trans->wait.comm->src_proc && trans->wait.comm->dst_proc){
          xbt_setset_set_insert(enabled, trans);
          DEBUG1("Transition %p is enabled for next state", trans);
        }
        break;

      /* WaitAny transitions are enabled if any of it's communications has both
         a sender and a receiver */
      case mc_waitany:
        xbt_dynar_foreach(trans->waitany.comms, index, comm){            
          if(comm->src_proc && comm->dst_proc){
            xbt_setset_set_insert(enabled, trans);
            DEBUG1("Transition %p is enabled for next state", trans);
            break;
          }
        }
        break;

      /* The rest of the transitions cannot be disabled */
      default:
        xbt_setset_set_insert(enabled, trans);
        DEBUG1("Transition %p is enabled for next state", trans);
        break;
    } 
  }
}

/**
 * \brief Determine if two transitions are dependent
 * \param t1 a transition
 * \param t2 a transition
 * \return TRUE if \a t1 and \a t2 are dependent. FALSE otherwise.
 */
int MC_transition_depend(mc_transition_t t1, mc_transition_t t2)
{
  if(t1->process == t2->process) 
    return FALSE;

  if(t1->type == mc_isend && t2->type == mc_irecv)
    return FALSE;

  if(t1->type == mc_irecv && t2->type == mc_isend)
    return FALSE;
  
  if(t1->type == mc_random || t2->type == mc_random)
    return FALSE;
  
  if(t1->type == mc_isend && t2->type == mc_isend && t1->isend.rdv != t2->isend.rdv)
    return FALSE;

  if(t1->type == mc_irecv && t2->type == mc_irecv && t1->irecv.rdv != t2->irecv.rdv)
    return FALSE;

  if(t1->type == mc_wait && t2->type == mc_wait && t1->wait.comm == t2->wait.comm)
    return FALSE;
  
  if(t1->type == mc_wait && t2->type == mc_wait 
     && t1->wait.comm->src_buff != NULL
     && t1->wait.comm->dst_buff != NULL
     && t2->wait.comm->src_buff != NULL
     && t2->wait.comm->dst_buff != NULL
     && t1->wait.comm->dst_buff != t2->wait.comm->src_buff
     && t1->wait.comm->dst_buff != t2->wait.comm->dst_buff
     && t2->wait.comm->dst_buff != t1->wait.comm->src_buff)
    return FALSE;

  return TRUE;
}