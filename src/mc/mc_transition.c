#include "private.h"

/**
 * \brief Create a model-checker transition and add it to the set of avaiable
 *        transitions of the current state, if not running in replay mode.
 * \param process The process who wants to perform the transition
 * \param comm The type of then transition (comm_send, comm_recv)
*/
mc_transition_t MC_create_transition(mc_trans_type_t type, smx_process_t p, smx_rdv_t rdv, smx_comm_t comm)
{
  mc_state_t current_state = NULL;
  char *type_str = NULL;
  
  if(!mc_replay_mode){
    MC_SET_RAW_MEM;
    mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);
    trans->refcount = 1;

    /* Generate a string for the "type" */
    switch(type){
      case mc_isend:
        type_str = bprintf("iSend");
        break;
      case mc_irecv:
        type_str = bprintf("iRecv");
        break;
      case mc_wait:
        type_str = bprintf("Wait");
        break;
      case mc_waitany:
        type_str = bprintf("WaitAny");
        break;
      default:
        xbt_die("Invalid transition type");
    }
    
    trans->name = bprintf("[%s][%s] %s (%p)", p->smx_host->name, p->name, type_str, trans);
    xbt_free(type_str);
    
    trans->type = type;
    trans->process = p;
    trans->rdv = rdv;
    trans->comm = comm;
    /* Push it onto the enabled transitions set of the current state */
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
    return trans;
  }
  return NULL;
}

void MC_random_create(int min, int max)
{
  smx_process_t p;
  mc_state_t current_state = NULL;
  char *type_str = NULL;
  
  if(!mc_replay_mode){
    p = SIMIX_process_self();
    
    MC_SET_RAW_MEM;
    mc_transition_t trans = xbt_new0(s_mc_transition_t, 1);
    
    trans->name = bprintf("[%s][%s] mc_random(%d,%d) (%p)", p->smx_host->name, p->name, min, max, trans);
    xbt_free(type_str);

    trans->refcount = 1;    
    trans->type = mc_random ;
    trans->process = p;
    trans->min = min;
    trans->max = max;
    trans->current_value = min;
    
    /* Push it onto the enabled transitions set of the current state */
    current_state = (mc_state_t) 
      xbt_fifo_get_item_content(xbt_fifo_get_first_item(mc_stack));
    xbt_setset_set_insert(current_state->transitions, trans);
    MC_UNSET_RAW_MEM;
  }
}

/** 
 * \brief Associate a communication to a transition
 * \param trans The transition
 * \param comm The communication
 */
void MC_transition_set_comm(mc_transition_t trans, smx_comm_t comm)
{
  if(trans)
    trans->comm = comm;
  return;
}

/**
 * \brief Deletes a transition
 * \param trans The transition to be deleted
 */
void MC_transition_delete(mc_transition_t trans)
{
  /* Only delete it if there are no references, otherwise decrement refcount */  
  if(--trans->refcount == 0){
    xbt_free(trans->name);
    xbt_free(trans);
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
  /* The semantics of SIMIX network operations implies that ONLY transitions 
     of the same type, in the same rendez-vous point, and from different processes
     are dependant, except wait transitions that are always independent */
  if(t1->process == t2->process) 
    return FALSE;

  if(t1->type == mc_isend && t2->type == mc_isend && t1->rdv == t2->rdv)
    return TRUE;

  if(t1->type == mc_irecv && t2->type == mc_irecv && t1->rdv == t2->rdv)
    return TRUE;

  if(t1->type == mc_wait && t2->type == mc_wait 
     && t1->comm->src_buff != NULL
     && t1->comm->dst_buff != NULL
     && t2->comm->src_buff != NULL
     && t2->comm->dst_buff != NULL
     && (   t1->comm->dst_buff == t2->comm->src_buff
         || t1->comm->dst_buff == t2->comm->dst_buff
         || t2->comm->dst_buff == t1->comm->src_buff))
    return TRUE;
  
  return FALSE;
}