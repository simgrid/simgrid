/* 	$Id$	 */

/* Copyright (c) 2009 Cristian Rosa.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix,
                                "Logging specific to SIMIX (network)");

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/ 

/**
 *  \brief Creates a new rendez-vous point
 *  \param name The name of the rendez-vous point
 *  \return The created rendez-vous point
 */
smx_rdv_t SIMIX_rdv_create(const char *name)
{
  smx_rdv_t rdv = xbt_new0(s_smx_rvpoint_t, 1);
  rdv->name = name ? xbt_strdup(name) : NULL;
  rdv->read = SIMIX_mutex_init();
  rdv->write = SIMIX_mutex_init();
  rdv->comm_fifo = xbt_fifo_new();

  return rdv;
}

/**
 *  \brief Destroy a rendez-vous point
 *  \param name The rendez-vous point to destroy
 */
void SIMIX_rdv_destroy(smx_rdv_t rdv)
{
  if(rdv->name)
    xbt_free(rdv->name);
  SIMIX_mutex_destroy(rdv->read);
  SIMIX_mutex_destroy(rdv->write);
  xbt_fifo_free(rdv->comm_fifo);
  xbt_free(rdv);
}

/**
 *  \brief Push a communication request into a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication request
 */
static inline void SIMIX_rdv_push(smx_rdv_t rdv, smx_comm_t comm)
{
  xbt_fifo_push(rdv->comm_fifo, comm);
  comm->rdv = rdv;
}

/**
 *  \brief Remove a communication request from a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication request
 */
static inline void SIMIX_rdv_remove(smx_rdv_t rdv, smx_comm_t comm)
{
  xbt_fifo_remove(rdv->comm_fifo, comm);
  comm->rdv = NULL;
}
  
/**
 *  \brief Checks if there is a communication request queued in a rendez-vous matching our needs
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication request if found, NULL otherwise.
 */
smx_comm_t SIMIX_rdv_get_request(smx_rdv_t rdv, smx_comm_type_t type)
{
  smx_comm_t comm = (smx_comm_t)xbt_fifo_get_item_content(
                                  xbt_fifo_get_first_item(rdv->comm_fifo));

  if(comm && comm->type == type){
    DEBUG0("Communication request found!");
    xbt_fifo_shift(rdv->comm_fifo);
    SIMIX_communication_use(comm);
    return comm;
  }

  /* no relevant request found. Return NULL */
  DEBUG0("Communication request not found");
  return NULL;
}

/**
 *  \brief counts the number of communication requests of a given host pending
 *         on a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param host The host to be counted
 *  \return The number of comm request pending in the rdv
 */
int 
SIMIX_rdv_get_count_waiting_comm(smx_rdv_t rdv, smx_host_t host)
{
  smx_comm_t comm = NULL;
  xbt_fifo_item_t item = NULL;
  int count = 0;

  xbt_fifo_foreach(rdv->comm_fifo, item, comm, smx_comm_t) {
    if (comm->src_proc->smx_host == host)
      count++;
  }

  return count;
}

/**
 *  \brief returns the communication at the head of the rendez-vous
 *  \param rdv The rendez-vous point
 *  \return The communication or NULL if empty
 */
smx_comm_t SIMIX_rdv_get_head(smx_rdv_t rdv)
{
  return (smx_comm_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(rdv->comm_fifo));
}


/******************************************************************************/
/*                           Communication Requests                           */
/******************************************************************************/ 

/**
 *  \brief Creates a new communication request
 *  \param type The type of communication (comm_send, comm_recv)
 *  \return The new communication request
 */  
smx_comm_t SIMIX_communication_new(smx_comm_type_t type)
{
  /* alloc structures */
  smx_comm_t comm = xbt_new0(s_smx_comm_t, 1);
  comm->type = type;
  comm->cond = SIMIX_cond_init();
  comm->refcount = 1;
  
  return comm;
}

/**
 *  \brief Destroy a communication request
 *  \param comm The request to be destroyed
 */
void SIMIX_communication_destroy(smx_comm_t comm)
{
  comm->refcount--;
  if(comm->refcount == 0){
    if(comm->act != NULL)
      SIMIX_action_destroy(comm->act);

    xbt_free(comm->cond);
    xbt_free(comm);
  }
}

/**
 *  \brief Increase the number of users of the communication.
 *  \param comm The communication request
 *  Each communication request can be used by more than one process, so it is
 *  necessary to know number of them at destroy time, to avoid freeing stuff that
 *  maybe is in use by others.
 *  \
 */
static inline void SIMIX_communication_use(smx_comm_t comm)
{
  comm->refcount++;
}

/**
 *  \brief Start the simulation of a communication request
 *  \param comm The communication request
 */
static inline void SIMIX_communication_start(smx_comm_t comm)
{
  /* If both the sender and the receiver are already there, start the communication */
  if(comm->src_proc && comm->dst_proc){
    DEBUG1("Starting communication %p", comm);
    comm->act = SIMIX_action_communicate(comm->src_proc->smx_host, 
                                         comm->dst_proc->smx_host, NULL, 
                                         comm->task_size, comm->rate);

    /* If any of the process is suspend, create the action but stop its execution,
       it will be restart when the sender process resume */
    if(SIMIX_process_is_suspended(comm->src_proc) || 
       SIMIX_process_is_suspended(comm->dst_proc)) {
      SIMIX_action_set_priority(comm->act, 0);
    }
    
    /* Add the communication as user data of the action */
    comm->act->data = comm;
    
    SIMIX_register_action_to_condition(comm->act, comm->cond);
  }else{
    DEBUG1("Communication %p cannot be started, peer missing", comm);
  }
}

/**
 *  \brief Waits for communication completion and performs error checking
 *  \param comm The communication
 *  \param timeout The max amount of time to wait for the communication to finish
 *
 *  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
static inline void SIMIX_communication_wait_for_completion(smx_comm_t comm, double timeout)
{
  xbt_ex_t e;

  DEBUG1("Waiting for the completion of communication %p", comm);
  
  if(timeout > 0){
    TRY{
      SIMIX_cond_wait_timeout(comm->cond, NULL, timeout);
    }
    CATCH(e){
      /* If it's a timeout then cancel the communication and signal the other peer */
      if(e.category == timeout_error){
        DEBUG1("Communication timeout! %p", comm);
        if(comm->act && SIMIX_action_get_state(comm->act) == SURF_ACTION_RUNNING)
          SIMIX_communication_cancel(comm);
        else
          SIMIX_rdv_remove(comm->rdv, comm);
          
        SIMIX_cond_signal(comm->cond);
        SIMIX_communication_destroy(comm);
      }
      RETHROW;
    }
  }else{
    SIMIX_cond_wait(comm->cond, NULL);
  }

  DEBUG1("Communication %p complete! Let's check for errors", comm);
  
  /* Check for errors */
  if(!SIMIX_host_get_state(SIMIX_host_self())){
    SIMIX_communication_destroy(comm);
    THROW0(host_error, 0, "Host failed");
  } else if (SIMIX_action_get_state(comm->act) == SURF_ACTION_FAILED){
    SIMIX_communication_destroy(comm);
    THROW0(network_error, 0, "Link failure");
  }

  SIMIX_unregister_action_to_condition(comm->act, comm->cond);
}

void SIMIX_communication_cancel(smx_comm_t comm)
{
  SIMIX_action_cancel(comm->act);
}

double SIMIX_communication_get_remains(smx_comm_t comm)
{
  return SIMIX_action_get_remains(comm->act);
}  

/**
 *  \brief Copy the communication data from the sender's buffer to the receiver's one
 *  \param comm The communication
 */
void SIMIX_network_copy_data(smx_comm_t comm)
{
  size_t src_buff_size = comm->src_buff_size;
  size_t dst_buff_size = *comm->dst_buff_size;
  
  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  dst_buff_size = MIN(dst_buff_size, src_buff_size);
  
  /* Update the receiver's buffer size to the copied amount */
  *comm->dst_buff_size = dst_buff_size;

  if(dst_buff_size == 0)
    return;

  memcpy(comm->dst_buff, comm->src_buff, dst_buff_size);

  DEBUG4("Copying comm %p data from %s -> %s (%zu bytes)", 
         comm, comm->src_proc->smx_host->name, comm->dst_proc->smx_host->name,
         dst_buff_size);
}

/**
 *  \brief Return the user data associated to the communication
 *  \param comm The communication
 *  \return the user data
 */
void *SIMIX_communication_get_data(smx_comm_t comm)
{
  return comm->data;
}

/******************************************************************************/
/*                        Synchronous Communication                           */
/******************************************************************************/
/*  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
void SIMIX_network_send(smx_rdv_t rdv, double task_size, double rate, 
                        double timeout, void *src_buff, size_t src_buff_size,
                        smx_comm_t *comm_ref, void *data)
{
  smx_comm_t comm;
  
  /* Look for communication request matching our needs. 
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, comm_recv);

  if(!comm){
    comm = SIMIX_communication_new(comm_send);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Update the communication reference with the comm to be used */
  *comm_ref = comm;
  
  /* Setup the communication request */
  comm->src_proc = SIMIX_process_self();
  comm->task_size = task_size;
  comm->rate = rate;
  comm->src_buff = src_buff;
  comm->src_buff_size = src_buff_size;
  comm->data = data;

  SIMIX_communication_start(comm);

  /* Wait for communication completion */
  SIMIX_communication_wait_for_completion(comm, timeout);

  SIMIX_communication_destroy(comm);
}

/*  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
void SIMIX_network_recv(smx_rdv_t rdv, double timeout, void *dst_buff, 
                        size_t *dst_buff_size, smx_comm_t *comm_ref)
{
  smx_comm_t comm;

  /* Look for communication request matching our needs. 
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, comm_send);

  if(!comm){
    comm = SIMIX_communication_new(comm_recv);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Update the communication reference with the comm to be used */
  *comm_ref = comm;
 
  /* Setup communication request */
  comm->dst_proc = SIMIX_process_self();
  comm->dst_buff = dst_buff;
  comm->dst_buff_size = dst_buff_size;

  SIMIX_communication_start(comm);

  /* Wait for communication completion */
  SIMIX_communication_wait_for_completion(comm, timeout);

  SIMIX_communication_destroy(comm);
}

/******************************************************************************/
/*                        Asynchronous Communication                          */
/******************************************************************************/

/*
void SIMIX_network_wait(smx_action_t comm, double timeout)
{
  TO BE IMPLEMENTED
}

XBT_PUBLIC(int) SIMIX_network_test(smx_action_t comm)
{
  TO BE IMPLEMENTED
}*/







