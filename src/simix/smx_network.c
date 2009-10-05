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
}

/**
 *  \brief Remove a communication request from a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication request
 */
static inline void SIMIX_rdv_remove(smx_rdv_t rdv, smx_comm_t comm)
{
  xbt_fifo_remove(rdv->comm_fifo, comm);
}
  

/**
 *  \brief Checks if there is a communication request queued in a rendez-vous matching our needs
 *  \param rdv The rendez-vous with the queue
 *  \param look_for_src boolean. True: we are receiver looking for sender; False: other way round
 *  \return The communication request if found, or a newly created one otherwise.
 */
smx_comm_t SIMIX_rdv_get_request(smx_rdv_t rdv, int (filter)(smx_comm_t, void*), void *arg) {
  smx_comm_t comm;
  xbt_fifo_item_t item;

  /* Traverse the rendez-vous queue looking for a comm request matching the
     filter conditions. If found return it and remove it from the list. */
  xbt_fifo_foreach(rdv->comm_fifo, item, comm, smx_comm_t) {
    if(filter(comm, arg)){
      SIMIX_communication_use(comm);
      xbt_fifo_remove_item(rdv->comm_fifo, item);
      DEBUG1("Communication request found! %p", comm);
      return comm;
    }
  }

  /* no relevant request found. Return NULL */
  DEBUG0("Communication request not found");
  return NULL;
}

/******************************************************************************/
/*                           Communication Requests                           */
/******************************************************************************/ 

/**
 *  \brief Creates a new communication request
 *  \param sender The process starting the communication (by send)
 *  \param receiver The process receiving the communication (by recv)
 *  \return the communication request
 */  
smx_comm_t SIMIX_communication_new(smx_comm_type_t type, smx_rdv_t rdv)
{
  /* alloc structures */
  smx_comm_t comm = xbt_new0(s_smx_comm_t, 1);
  comm->type = type;
  comm->cond = SIMIX_cond_init();
  comm->rdv = rdv;
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
  if(comm->src_host != NULL && comm->dst_host != NULL){
    DEBUG1("Starting communication %p", comm);
    comm->act = SIMIX_action_communicate(comm->src_host, comm->dst_host, NULL, 
                                         comm->task_size, comm->rate);

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
          SIMIX_action_cancel(comm->act);
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

  memcpy(comm->dst_buff, comm->src_buff, dst_buff_size);

  DEBUG4("Copying comm %p data from %s -> %s (%zu bytes)", 
         comm, comm->src_host->name, comm->dst_host->name, dst_buff_size);
}

/* FIXME: move to some other place */
int comm_filter_get(smx_comm_t comm, void *arg)
{
  if(comm->type == comm_send){
    if(arg && comm->src_host != (smx_host_t)arg)
     return FALSE;
    else
     return TRUE;
  }else{
    return FALSE;
  }
}

int comm_filter_put(smx_comm_t comm, void *arg)
{
  return comm->type == comm_recv ? TRUE : FALSE;
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
                        double timeout, void *data, size_t data_size,
                        int (filter)(smx_comm_t, void *), void *arg)
{
  smx_comm_t comm;
  
  /* Look for communication request matching our needs. 
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, filter, arg);

  if(comm == NULL){
    comm = SIMIX_communication_new(comm_send, rdv);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Setup the communication request */
  comm->src_host = SIMIX_host_self();
  comm->task_size = task_size;
  comm->rate = rate;
  comm->src_buff = data;
  comm->src_buff_size = data_size;

  SIMIX_communication_start(comm);

  /* Wait for communication completion */
  /* FIXME: if the semantic is non blocking, it shouldn't wait on the condition here */
  SIMIX_communication_wait_for_completion(comm, timeout);

  SIMIX_communication_destroy(comm);
}

/*  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
void SIMIX_network_recv(smx_rdv_t rdv, double timeout, void *data, 
                        size_t *data_size, int (filter)(smx_comm_t, void *), void *arg)
{
  smx_comm_t comm;

  /* Look for communication request matching our needs. 
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, filter, arg);

  if(comm == NULL){
    comm = SIMIX_communication_new(comm_recv, rdv);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Setup communication request */
  comm->dst_host = SIMIX_host_self();
  comm->dst_buff = data;
  comm->dst_buff_size = data_size;

  SIMIX_communication_start(comm);

  /* Wait for communication completion */
  /* FIXME: if the semantic is non blocking, it shouldn't wait on the condition here */
  SIMIX_communication_wait_for_completion(comm, timeout);

  SIMIX_communication_destroy(comm);
}

/******************************************************************************/
/*                        Asynchronous Communication                          */
/******************************************************************************/

/*
void SIMIX_network_wait(smx_action_t comm, double timeout)
{
    if (timeout > 0)
      SIMIX_cond_wait_timeout(rdv_cond, rdv_comm_mutex, timeout - start_time);
    else
      SIMIX_cond_wait(rdv_cond, rdv_comm_mutex);    

}


XBT_PUBLIC(int) SIMIX_network_test(smx_action_t comm)
{
  if(SIMIX_action_get_state (comm) == SURF_ACTION_DONE){
    memcpy(comm->data

  return SIMIX_action_get_state (comm) == SURF_ACTION_DONE ? TRUE : FALSE;
}*/







