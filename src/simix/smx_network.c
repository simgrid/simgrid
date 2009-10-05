/* 	$Id$	 */

/* Copyright (c) 2009 Cristian Rosa.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/ 

/**
 *  \brief Creates a new rendez-vous point
 *  \param name The name of the rendez-vous point
 *  \return The created rendez-vous point
 */
smx_rdv_t SIMIX_rdv_create(char *name)
{
  smx_rdv_t rvp = xbt_new0(s_smx_rvpoint_t, 1);
  rvp->name = xbt_strdup(name);
  rvp->read = SIMIX_mutex_init();
  rvp->write = SIMIX_mutex_init();
  rvp->comm_mutex = SIMIX_mutex_init();
  rvp->comm_fifo = xbt_fifo_new();

  return rvp;
}

/**
 *  \brief Destroy a rendez-vous point
 *  \param name The rendez-vous point to destroy
 */
void SIMIX_rdv_destroy(smx_rdv_t rvp)
{
  xbt_free(rvp->name);
  SIMIX_mutex_destroy(rvp->read);
  SIMIX_mutex_destroy(rvp->write);
  SIMIX_mutex_destroy(rvp->comm_mutex);
  xbt_fifo_free(rvp->comm_fifo);
  xbt_free(rvp);
}

/**
 *  \brief Push a communication request into a rendez-vous point
 *  The communications request are dequeued by the two functions below
 *  \param rvp The rendez-vous point
 *  \param comm The communication request
 */
static inline void SIMIX_rdv_push(smx_rdv_t rvp, smx_comm_t comm)
{
  xbt_fifo_push(rvp->comm_fifo, comm);
}

/**
 *  \brief Checks if there is a communication request queued in a rendez-vous matching our needs
 *  \param rvp The rendez-vous with the queue
 *  \param look_for_src boolean. True: we are receiver looking for sender; False: other way round
 *  \return The communication request if found, a newly created one otherwise.
 */
smx_comm_t SIMIX_rdv_get_request_or_create(smx_rdv_t rvp, int look_for_src) {
  /* Get a communication request from the rendez-vous queue. If it is a receive
     request then return it, otherwise put it again in the queue and return NULL
   */
  smx_comm_t comm_head = xbt_fifo_shift(rvp->comm_fifo);

  if(comm_head != NULL) {
    if (( look_for_src && comm_head->src_host != NULL) ||
        (!look_for_src && comm_head->dst_host != NULL)) {

      SIMIX_communication_use(comm_head);
      return comm_head;
    }
  }
  xbt_fifo_unshift(rvp->comm_fifo, comm_head);

  /* no relevant peer found. Create a new comm action and set it up */
  comm_head = SIMIX_communication_new(rvp);
  SIMIX_rdv_push(rvp, comm_head);
  SIMIX_communication_use(comm_head);

  return comm_head;
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
smx_comm_t SIMIX_communication_new(smx_rdv_t rdv)
{
  /* alloc structures */
  smx_comm_t comm = xbt_new0(s_smx_comm_t, 1);
  comm->cond = SIMIX_cond_init();

  comm->rdv = rdv;

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

    if(comm->data != NULL)
      xbt_free(comm->data);

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
 *  \param comm The communicatino request
 */
static inline void SIMIX_communication_start(smx_comm_t comm)
{
  comm->act = SIMIX_action_communicate(comm->src_host, comm->dst_host, NULL, 
      comm->data_size, comm->rate);

  SIMIX_register_action_to_condition(comm->act, comm->cond);
}

static inline void SIMIX_communication_wait_for_completion(smx_comm_t comm)
{
  __SIMIX_cond_wait(comm->cond);
  if (comm->act) {
    SIMIX_unregister_action_to_condition(comm->act, comm->cond);
    SIMIX_action_destroy(comm->act);
    comm->act=NULL;
  }
  /* Check for errors */
  if (SIMIX_host_get_state(comm->dst_host) == 0){
    THROW1(host_error, 0, "Destination host %s failed", comm->dst_host->name);
  } else if (SIMIX_host_get_state(comm->src_host) == 0){
    THROW1(host_error, 0, "Source host %s failed", comm->src_host->name);
  } else if (SIMIX_action_get_state(comm->act) == SURF_ACTION_FAILED) {
    THROW0(network_error, 0, "Link failure");
  }

}
/******************************************************************************/
/*                        Synchronous Communication                           */
/******************************************************************************/
/* Throws:
 *  - host_error if peer failed
 *  - network_error if network failed */

void SIMIX_network_send(smx_rdv_t rdv, void *data, size_t size, double timeout, double rate)
{
  /*double start_time = SIMIX_get_clock();*/
  void *smx_net_data;
  smx_comm_t comm;

  /* Copy the message to the network */
  /*FIXME here the MC should allocate the space from the network storage area */
  smx_net_data = xbt_malloc(size);
  memcpy(smx_net_data, data, size);

  comm = SIMIX_rdv_get_request_or_create(rdv,0);
  comm->src_host = SIMIX_host_self();
  comm->data = smx_net_data;
  comm->data_size = size;
  comm->rate = rate;

  /* If the receiver is already there, start the communication */
  /* If it's not here already, it will start that communication itself later on */
  if(comm->dst_host != NULL)
    SIMIX_communication_start(comm);

  /* Wait for communication completion */
  /* FIXME: if the semantic is non blocking, it shouldn't wait on the condition here */
  /* FIXME: add timeout checking stuff */
  SIMIX_communication_wait_for_completion(comm);

  SIMIX_communication_destroy(comm);
}

void SIMIX_network_recv(smx_rdv_t rdv, void **data, size_t *size, double timeout)
{
  /*double start_time = SIMIX_get_clock();*/
  smx_comm_t comm;
  smx_host_t my_host = SIMIX_host_self();

  comm = SIMIX_rdv_get_request_or_create(rdv,1);
  comm->dst_host = SIMIX_host_self();
  comm->dest_buff = data;

  /* If the sender is already there, start the communication */
  /* If it's not here already, it will start that communication itself later on */
  if (comm->src_host != NULL)
    SIMIX_communication_start(comm);

  /* Wait for communication completion */
  /* FIXME: if the semantic is non blocking, it shouldn't wait on the condition here */
  /* FIXME: add timeout checking stuff */
  SIMIX_communication_wait_for_completion(comm);

  /* We are OK, let's copy the message to receiver's buffer */
  *size = *size < comm->data_size ? *size : comm->data_size;
  memcpy(*data, comm->data, *size);

  SIMIX_communication_destroy(comm);
}

/******************************************************************************/
/*                        Asynchronous Communication                          */
/******************************************************************************/

/*
void SIMIX_network_wait(smx_action_t comm, double timeout)
{
    if (timeout > 0)
      SIMIX_cond_wait_timeout(rvp_cond, rvp_comm_mutex, timeout - start_time);
    else
      SIMIX_cond_wait(rvp_cond, rvp_comm_mutex);    

}


XBT_PUBLIC(int) SIMIX_network_test(smx_action_t comm)
{
  if(SIMIX_action_get_state (comm) == SURF_ACTION_DONE){
    memcpy(comm->data

  return SIMIX_action_get_state (comm) == SURF_ACTION_DONE ? TRUE : FALSE;
}*/







