/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gos, msg,
				"Logging specific to MSG (gos)");

/** \ingroup msg_gos_functions
 * \brief This function is now deprecated and useless. Please stop using it.
 */

MSG_error_t MSG_process_start(m_process_t process)
{
  xbt_assert0(0,"This function is now deprecated and useless. Please stop using it.");
  
  return MSG_OK;
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task.
 *
 * It takes two parameter.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
 * if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get(m_task_t * task,
			 m_channel_t channel)
{
  m_process_t process = MSG_process_self();
  m_task_t t = NULL;
  m_host_t h = NULL;
  simdata_task_t t_simdata = NULL;
  simdata_host_t h_simdata = NULL;
  int warning = 0;
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  
  CHECK_HOST();
  /* Sanity check */
  xbt_assert0(task,"Null pointer for the task\n");

  if (*task) 
    CRITICAL0("MSG_task_get() was asked to write in a non empty task struct.");

  /* Get the task */
  h = MSG_host_self();
  h_simdata = h->simdata;
  while ((t = xbt_fifo_pop(h_simdata->mbox[channel])) == NULL) {
    xbt_assert0(!(h_simdata->sleeping[channel]),
		"A process is already blocked on this channel");
    h_simdata->sleeping[channel] = process; /* I'm waiting. Wake me up when you're ready */
    MSG_process_suspend(process);
    if(surf_workstation_resource->extension_public->get_state(h_simdata->host) 
       == SURF_CPU_OFF)
      MSG_RETURN(MSG_HOST_FAILURE);
    h_simdata->sleeping[channel] = NULL;
    /* OK, we should both be ready now. Are you there ? */
  }

  t_simdata = t->simdata;
  /*   *task = __MSG_task_copy(t); */
  *task=t;

  /* Transfer */
  t_simdata->using++;
  t_simdata->comm = surf_workstation_resource->extension_public->
    communicate(MSG_process_get_host(t_simdata->sender)->simdata->host,
		h->simdata->host, t_simdata->message_size);
  surf_workstation_resource->common_public->action_set_data(t_simdata->comm,t);

  if(MSG_process_isSuspended(t_simdata->sender)) 
    MSG_process_resume(t_simdata->sender);

  do {
    __MSG_task_wait_event(process, t);
    state=surf_workstation_resource->common_public->action_get_state(t_simdata->comm);
  } while (state==SURF_ACTION_RUNNING);

  if(state == SURF_ACTION_DONE) MSG_RETURN(MSG_OK);
  else if(surf_workstation_resource->extension_public->get_state(h_simdata->host) 
	  == SURF_CPU_OFF)
    MSG_RETURN(MSG_HOST_FAILURE);
  else MSG_RETURN(MSG_TRANSFER_FAILURE);
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return 1 if there is a pending communication and 0 otherwise
 */
int MSG_task_Iprobe(m_channel_t channel)
{
  m_host_t h = NULL;
  simdata_host_t h_simdata = NULL;

  CHECK_HOST();
  h = MSG_host_self();
  h_simdata = h->simdata;
  return(xbt_fifo_getFirstItem(h_simdata->mbox[channel])!=NULL);
}

/** \ingroup msg_gos_functions
 * \brief Put a task on a channel of an host and waits for the end of the
 * transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes three parameter.
 * \param task a #m_task_t to send on another location. This task
   will not be usable anymore when the function will return. There is
   no automatic task duplication and you have to save your parameters
   before calling this function. Tasks are unique and once it has been
   sent to another location, you should not access it anymore. You do
   not need to call MSG_task_destroy() but to avoid using, as an
   effect of inattention, this task anymore, you definitely should
   renitialize it with #MSG_TASK_UNINITIALIZED. Note that this task
   can be transfered iff it has been correctly created with
   MSG_task_create().
 * \param dest the destination of the message
 * \param channel the channel on which the agent should put this
   task. This value has to be >=0 and < than the maximal number of
   channels fixed with MSG_set_channel_number().
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise.
 */
MSG_error_t MSG_task_put(m_task_t task,
			 m_host_t dest, m_channel_t channel)
{
  m_process_t process = MSG_process_self();
  simdata_task_t task_simdata = NULL;
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  m_host_t local_host = NULL;
  m_host_t remote_host = NULL;

  CHECK_HOST();

  task_simdata = task->simdata;
  task_simdata->sender = process;
  xbt_assert0(task_simdata->using==1,"Gargl!");
  task_simdata->comm = NULL;
  
  local_host = ((simdata_process_t) process->simdata)->host;
  remote_host = dest;

  xbt_fifo_push(((simdata_host_t) remote_host->simdata)->
		mbox[channel], task);
    
  if(remote_host->simdata->sleeping[channel]) 
    MSG_process_resume(remote_host->simdata->sleeping[channel]);
  else {
    process->simdata->put_host = dest;
    process->simdata->put_channel = channel;
    while(!(task_simdata->comm)) 
      MSG_process_suspend(process);
    process->simdata->put_host = NULL;
    process->simdata->put_channel = -1;
  }

  do {
    __MSG_task_wait_event(process, task);
    state=surf_workstation_resource->common_public->action_get_state(task_simdata->comm);
  } while (state==SURF_ACTION_RUNNING);
    
  MSG_task_destroy(task);

  if(state == SURF_ACTION_DONE) MSG_RETURN(MSG_OK);
  else if(surf_workstation_resource->extension_public->get_state(local_host->simdata->host) 
	  == SURF_CPU_OFF)
    MSG_RETURN(MSG_HOST_FAILURE);
  else MSG_RETURN(MSG_TRANSFER_FAILURE);
}

/** \ingroup msg_gos_functions
 * \brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of an agent. It
 * takes only one parameter.
 * \param task a #m_task_t to execute on the location on which the
   agent is running.
 * \return #MSG_FATAL if \a task is not properly initialized and
 * #MSG_OK otherwise.
 */
MSG_error_t MSG_task_execute(m_task_t task)
{
  m_process_t process = MSG_process_self();

  __MSG_task_execute(process, task);
  return __MSG_wait_for_computation(process,task);
}

void __MSG_task_execute(m_process_t process, m_task_t task)
{
  simdata_task_t simdata = NULL;

  CHECK_HOST();

  simdata = task->simdata;

  simdata->compute = surf_workstation_resource->extension_public->
    execute(MSG_process_get_host(process)->simdata->host,
	    simdata->computation_amount);
  surf_workstation_resource->common_public->action_set_data(simdata->compute,task);
}

MSG_error_t __MSG_wait_for_computation(m_process_t process, m_task_t task)
{
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  simdata_task_t simdata = task->simdata;

  simdata->using++;
  do {
    __MSG_task_wait_event(process, task);
    state=surf_workstation_resource->common_public->action_get_state(simdata->compute);
  } while (state==SURF_ACTION_RUNNING);
  simdata->using--;
    

  if(state == SURF_ACTION_DONE) MSG_RETURN(MSG_OK);
  else if(surf_workstation_resource->extension_public->
	  get_state(MSG_process_get_host(process)->simdata->host) 
	  == SURF_CPU_OFF)
    MSG_RETURN(MSG_HOST_FAILURE);
  else MSG_RETURN(MSG_TRANSFER_FAILURE);
}

/** \ingroup msg_gos_functions
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
MSG_error_t MSG_process_sleep(long double nb_sec)
{
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  m_process_t process = MSG_process_self();
  m_task_t dummy = NULL;
  simdata_task_t simdata = NULL;

  CHECK_HOST();
  dummy = MSG_task_create("MSG_sleep", nb_sec, 0.0, NULL);
  simdata = dummy->simdata;

  simdata->compute = surf_workstation_resource->extension_public->
    sleep(MSG_process_get_host(process)->simdata->host,
	    simdata->computation_amount);
  surf_workstation_resource->common_public->action_set_data(simdata->compute,dummy);

  
  simdata->using++;
  do {
    __MSG_task_wait_event(process, dummy);
    state=surf_workstation_resource->common_public->action_get_state(simdata->compute);
  } while (state==SURF_ACTION_RUNNING);
  simdata->using--;
    
  if(state == SURF_ACTION_DONE) {
    if(surf_workstation_resource->extension_public->
       get_state(MSG_process_get_host(process)->simdata->host) 
       == SURF_CPU_OFF)
      MSG_RETURN(MSG_HOST_FAILURE);

    if(MSG_process_isSuspended(process)) {
      MSG_process_suspend(MSG_process_self());
    }
    if(surf_workstation_resource->extension_public->
       get_state(MSG_process_get_host(process)->simdata->host) 
       == SURF_CPU_OFF)
      MSG_RETURN(MSG_HOST_FAILURE);
    MSG_task_destroy(dummy);
    MSG_RETURN(MSG_OK);
  } else MSG_RETURN(MSG_HOST_FAILURE);
}

/** \ingroup msg_gos_functions
 * \brief Return the number of MSG tasks currently running on a
 * the host of the current running process.
 */
int MSG_get_msgload(void) 
{
  CHECK_HOST();
  xbt_assert0(0,"Not implemented yet!");
  
  return 1;
}

/** \ingroup msg_gos_functions
 *
 * \brief Return the the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
MSG_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
}

