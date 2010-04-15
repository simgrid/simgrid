/* 	$Id$	 */

/* Copyright (c) 2009 Cristian Rosa.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"
#include "xbt/dict.h"

/* Pimple to get an histogram of message sizes in the simulation */
xbt_dict_t msg_sizes = NULL;

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
smx_comm_t SIMIX_rdv_get_request(smx_rdv_t rdv, smx_comm_type_t type) {
  smx_comm_t comm = (smx_comm_t)xbt_fifo_get_item_content(
                                  xbt_fifo_get_first_item(rdv->comm_fifo));

  if(comm && comm->type == type){
    DEBUG0("Communication request found!");
    xbt_fifo_shift(rdv->comm_fifo);
    SIMIX_communication_use(comm);
    comm->rdv = NULL;    
    return comm;
  }

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
XBT_INLINE smx_comm_t SIMIX_rdv_get_head(smx_rdv_t rdv)
{
  return (smx_comm_t)xbt_fifo_get_item_content(xbt_fifo_get_first_item(rdv->comm_fifo));
}

/** @brief adds some API-related data to the rendez-vous point */
XBT_INLINE void SIMIX_rdv_set_data(smx_rdv_t rdv,void *data) {
  rdv->data=data;
}
/** @brief gets API-related data from the rendez-vous point */
XBT_INLINE void *SIMIX_rdv_get_data(smx_rdv_t rdv) {
  return rdv->data;
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
  comm->sem = SIMIX_sem_init(0);
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
  if(comm->refcount > 0)
    return;

  if(comm->sem){
    SIMIX_sem_destroy(comm->sem);
    comm->sem = NULL;
  }
  
  if(comm->act){
    SIMIX_action_destroy(comm->act);
    comm->act = NULL;
  }

  if(comm->src_timeout){
    SIMIX_action_destroy(comm->src_timeout);
    comm->src_timeout = NULL;
  }

  if(comm->dst_timeout){
      SIMIX_action_destroy(comm->dst_timeout);
      comm->dst_timeout = NULL;
    }

  xbt_free(comm);
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
 *  \param comm The   comm->rdv = NULL;communication request
 */
static inline void SIMIX_communication_start(smx_comm_t comm)
{
  /* If both the sender and the receiver are already there, start the communication */
  if(comm->src_proc && comm->dst_proc){
    DEBUG1("Starting communication %p", comm);
    comm->act = SIMIX_action_communicate(comm->src_proc->smx_host, 
                                         comm->dst_proc->smx_host, NULL, 
                                         comm->task_size, comm->rate);
#ifdef HAVE_TRACING
    TRACE_smx_action_communicate (comm->act, comm->src_proc);
#endif

    /* If any of the process is suspend, create the action but stop its execution,
       it will be restarted when the sender process resume */
    if(SIMIX_process_is_suspended(comm->src_proc) || 
       SIMIX_process_is_suspended(comm->dst_proc)) {
      SIMIX_action_suspend(comm->act);
    }
    
    /* Add the communication as user data of the action */
    comm->act->data = comm;

    /* The semaphore will only get signaled once, but since the first unlocked guy will
     * release_forever() the semaphore, that will unlock the second (and any other)
     * communication partner */
    SIMIX_register_action_to_semaphore(comm->act, comm->sem);
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
  smx_action_t act_sleep = NULL;
  int src_timeout = 0;
  int dst_timeout = 0;

  DEBUG1("Waiting for the completion of communication %p", comm);
  
  if (timeout >= 0) {
    act_sleep = SIMIX_action_sleep(SIMIX_host_self(), timeout);
		if(SIMIX_process_self()==comm->src_proc)
			comm->src_timeout = act_sleep;
		else
			comm->dst_timeout = act_sleep;
    SIMIX_action_set_name(act_sleep,bprintf("Timeout for comm %p and wait on semaphore %p (max_duration:%f)", comm, comm->sem,timeout));
    SIMIX_register_action_to_semaphore(act_sleep, comm->sem);
    SIMIX_process_self()->waiting_action = act_sleep;
    SIMIX_sem_block_onto(comm->sem);
    SIMIX_process_self()->waiting_action = NULL;
    SIMIX_unregister_action_to_semaphore(act_sleep, comm->sem);
  } else {
    SIMIX_sem_acquire(comm->sem);
  }

  /* Check for timeouts */
  if ((src_timeout = ((comm->src_timeout) && (SIMIX_action_get_state(comm->src_timeout) == SURF_ACTION_DONE))) ||
      (dst_timeout = ((comm->dst_timeout) && (SIMIX_action_get_state(comm->dst_timeout) == SURF_ACTION_DONE))) ) {
			/* Somebody did a timeout! */
    if (src_timeout) DEBUG1("Communication timeout from the src! %p", comm);
    if (dst_timeout) DEBUG1("Communication timeout from the dst! %p", comm);

    if(comm->act && SIMIX_action_get_state(comm->act) == SURF_ACTION_RUNNING)
      SIMIX_communication_cancel(comm);
    else if (comm->rdv)
      SIMIX_rdv_remove(comm->rdv, comm);

    /* Make sure that everyone sleeping on that semaphore is awake, and that nobody will ever block on it */
    SIMIX_sem_release_forever(comm->sem);
    SIMIX_communication_destroy(comm);

    THROW1(timeout_error, 0, "Communication timeouted because of %s",src_timeout?"the source":"the destination");
  }

  DEBUG1("Communication %p complete! Let's check for errors", comm);

  /* Make sure that everyone sleeping on that semaphore is awake, and that nobody will ever block on it */
  SIMIX_sem_release_forever(comm->sem);
  
  /* Check for errors other than timeouts (they are catched above) */
  if(!SIMIX_host_get_state(SIMIX_host_self())){
    if(comm->rdv)
      SIMIX_rdv_remove(comm->rdv, comm);
    SIMIX_communication_destroy(comm);
    THROW0(host_error, 0, "Host failed");
  } else if (SIMIX_action_get_state(comm->act) == SURF_ACTION_FAILED){
    SIMIX_communication_destroy(comm);
    THROW0(network_error, 0, "Link failure");
  }
  SIMIX_communication_destroy(comm);
}

/**
 *  \brief Cancels a communication
 *  \brief comm The communication to cancel
 */
XBT_INLINE void SIMIX_communication_cancel(smx_comm_t comm)
{
  if (comm->act)
    SIMIX_action_cancel(comm->act);
}

/**
 *  \brief get the amount remaining from the communication
 *  \param comm The communication
 */
XBT_INLINE double SIMIX_communication_get_remains(smx_comm_t comm)
{
  return SIMIX_action_get_remains(comm->act);
}  

/******************************************************************************/
/*                    SIMIX_network_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_network_copy_data_callback)(smx_comm_t, size_t) = &SIMIX_network_copy_pointer_callback;

void SIMIX_network_set_copy_data_callback(void (*callback)(smx_comm_t, size_t)) {
  SIMIX_network_copy_data_callback = callback;
}

void SIMIX_network_copy_pointer_callback(smx_comm_t comm, size_t buff_size) {
  xbt_assert1((buff_size == sizeof(void*)), "Cannot copy %zu bytes: must be sizeof(void*)",buff_size);
  *(void**)(comm->dst_buff) = comm->src_buff;
}

void SIMIX_network_copy_buffer_callback(smx_comm_t comm, size_t buff_size) {
  memcpy(comm->dst_buff, comm->src_buff, buff_size);
}

/**
 *  \brief Copy the communication data from the sender's buffer to the receiver's one
 *  \param comm The communication
 */
void SIMIX_network_copy_data(smx_comm_t comm)
{
  size_t buff_size = comm->src_buff_size;

  DEBUG6("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)",
      comm,
      comm->src_proc->smx_host->name, comm->src_buff,
      comm->dst_proc->smx_host->name, comm->dst_buff,
      buff_size);

  /* If there is no data to be copy then return */
  if(!comm->src_buff || !comm->dst_buff)
    return;
  
  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (comm->dst_buff_size)
    buff_size = MIN(buff_size,*(comm->dst_buff_size));
  
  /* Update the receiver's buffer size to the copied amount */
  if (comm->dst_buff_size)
    *comm->dst_buff_size = buff_size;

  if(buff_size == 0)
    return;
  (*SIMIX_network_copy_data_callback)(comm, buff_size);

  /* pimple to display the message sizes */
  {
    if (msg_sizes == NULL)
      msg_sizes = xbt_dict_new();
    uintptr_t casted_size = comm->task_size;
    uintptr_t amount = xbt_dicti_get(msg_sizes, casted_size);
    amount++;

    xbt_dicti_set(msg_sizes,casted_size, amount);
  }
}
#include "xbt.h"
/* pimple to display the message sizes */
void SIMIX_message_sizes_output(const char *filename) {
  FILE * out = fopen(filename,"w");
  xbt_assert1(out,"Cannot open file %s",filename);
  uintptr_t key,data;
  xbt_dict_cursor_t cursor;
  xbt_dict_foreach(msg_sizes,cursor,key,data) {
    fprintf(out,"%zu %zu\n",key,data);
  }
  fclose(out);
}

/**
 *  \brief Return the user data associated to the communication
 *  \param comm The communication
 *  \return the user data
 */
XBT_INLINE void *SIMIX_communication_get_data(smx_comm_t comm)
{
  return comm->data;
}

/******************************************************************************/
/*                        Synchronous Communication                           */
/******************************************************************************/
/**
 *  \brief Put a send communication request in a rendez-vous point and waits for
 *         its completion (blocking)
 *  \param rdv The rendez-vous point
 *  \param task_size The size of the communication action (for surf simulation)
 *  \param rate The rate of the communication action (for surf)
 *  \param timeout The timeout used for the waiting the completion 
 *  \param src_buff The source buffer containing the message to be sent
 *  \param src_buff_size The size of the source buffer
 *  \param comm_ref The communication object used for the send  (useful if someone else wants to cancel this communication afterward)
 *  \param data User data associated to the communication object
 *  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
XBT_INLINE void SIMIX_network_send(smx_rdv_t rdv, double task_size, double rate,
                        double timeout, void *src_buff, size_t src_buff_size,
                        smx_comm_t *comm_ref, void *data)
{
  *comm_ref = SIMIX_network_isend(rdv,task_size,rate,src_buff,src_buff_size,data);
  SIMIX_network_wait(*comm_ref,timeout);
}

/**
 *  \brief Put a receive communication request in a rendez-vous point and waits
 *         for its completion (blocking)
 *  \param rdv The rendez-vous point
 *  \param timeout The timeout used for the waiting the completion 
 *  \param dst_buff The destination buffer to copy the received message
 *  \param src_buff_size The size of the destination buffer
 *  \param comm_ref The communication object used for the send (useful if someone else wants to cancel this communication afterward)
 *  Throws:
 *   - host_error if peer failed
 *   - timeout_error if communication reached the timeout specified
 *   - network_error if network failed or peer issued a timeout
 */
XBT_INLINE void SIMIX_network_recv(smx_rdv_t rdv, double timeout, void *dst_buff,
                        size_t *dst_buff_size, smx_comm_t *comm_ref)
{
  *comm_ref = SIMIX_network_irecv(rdv,dst_buff,dst_buff_size);
  SIMIX_network_wait(*comm_ref,timeout);
}

/******************************************************************************/
/*                        Asynchronous Communication                          */
/******************************************************************************/
smx_comm_t SIMIX_network_isend(smx_rdv_t rdv, double task_size, double rate,
    void *src_buff, size_t src_buff_size, void *data)
{
  smx_comm_t comm;

  /* Look for communication request matching our needs.
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, comm_recv);

  if(!comm){
    comm = SIMIX_communication_new(comm_send);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Setup the communication request */
  comm->src_proc = SIMIX_process_self();
  comm->task_size = task_size;
  comm->rate = rate;
  comm->src_buff = src_buff;
  comm->src_buff_size = src_buff_size;
  comm->data = data;

  SIMIX_communication_start(comm);
  return comm;
}

smx_comm_t SIMIX_network_irecv(smx_rdv_t rdv, void *dst_buff, size_t *dst_buff_size) {
  smx_comm_t comm;

  /* Look for communication request matching our needs.
     If it is not found then create it and push it into the rendez-vous point */
  comm = SIMIX_rdv_get_request(rdv, comm_send);

  if(!comm){
    comm = SIMIX_communication_new(comm_recv);
    SIMIX_rdv_push(rdv, comm);
  }

  /* Setup communication request */
  comm->dst_proc = SIMIX_process_self();
  comm->dst_buff = dst_buff;
  comm->dst_buff_size = dst_buff_size;

  SIMIX_communication_start(comm);
  return comm;
}

/** @brief blocks until the communication terminates or the timeout occurs */
XBT_INLINE void SIMIX_network_wait(smx_comm_t comm, double timeout) {
  /* Wait for communication completion */
  SIMIX_communication_wait_for_completion(comm, timeout);
}

/** @Returns whether the (asynchronous) communication is done yet or not */
XBT_INLINE int SIMIX_network_test(smx_comm_t comm) {
  return comm->sem?SIMIX_sem_would_block(comm->sem):0;
}

/** @brief wait for the completion of any communication of a set
 *
 *  @Returns the rank in the dynar of communication which finished; destroy it after identifying which one it is
 */
unsigned int SIMIX_network_waitany(xbt_dynar_t comms) {
  xbt_dynar_t sems = xbt_dynar_new(sizeof(smx_sem_t),NULL);
  unsigned int cursor, found_comm=-1;
  smx_comm_t comm,comm_finished=NULL;

  xbt_dynar_foreach(comms,cursor,comm){
    xbt_dynar_push(sems,&(comm->sem));
  }

  DEBUG1("Waiting for the completion of communication set %p", comms);

  found_comm = SIMIX_sem_acquire_any(sems);
  xbt_assert0(found_comm!=-1,"Cannot find which communication finished");
  xbt_dynar_get_cpy(comms,found_comm,&comm_finished);

  DEBUG1("Communication %p complete! Let's check for errors", comm_finished);

  /* Make sure that everyone sleeping on that semaphore is awake,
   * and that nobody will ever block on it */
  SIMIX_sem_release_forever(comm_finished->sem);

  /* Check for errors */
  if(!SIMIX_host_get_state(SIMIX_host_self())){
    if(comm_finished->rdv)
      SIMIX_rdv_remove(comm_finished->rdv, comm_finished);
    SIMIX_communication_destroy(comm_finished);
    THROW0(host_error, 0, "Host failed");
  } else if (SIMIX_action_get_state(comm_finished->act) == SURF_ACTION_FAILED){
    SIMIX_communication_destroy(comm_finished);
    THROW0(network_error, 0, "Link failure");
  }
  SIMIX_communication_destroy(comm_finished);

  return found_comm;
}
