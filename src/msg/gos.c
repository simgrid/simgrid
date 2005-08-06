/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gos, msg,
				"Logging specific to MSG (gos)");

/** \defgroup msg_gos_functions MSG Operating System Functions
 *  \brief This section describes the functions that can be used
 *  by an agent for handling some task.
 */

/* \ingroup msg_gos_functions
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
 * It takes two parameters.
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
  return MSG_task_get_with_time_out(task, channel, -1);
}

/** \ingroup msg_gos_functions
 * \brief Listen on a channel and wait for receiving a task with a timeout.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param max_duration the maximum time to wait for a task before giving
    up. In such a case, \a task will not be modified and will still be
    equal to \c NULL when returning.
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
   if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */

MSG_error_t MSG_task_get_with_time_out(m_task_t * task,
				       m_channel_t channel,
				       double max_duration)
{
  m_process_t process = MSG_process_self();
  m_task_t t = NULL;
  m_host_t h = NULL;
  simdata_task_t t_simdata = NULL;
  simdata_host_t h_simdata = NULL;
  int first_time = 1;
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  
  CHECK_HOST();
  /* Sanity check */
  xbt_assert0(task,"Null pointer for the task\n");

  if (*task) 
    CRITICAL0("MSG_task_get() was asked to write in a non empty task struct.");

  /* Get the task */
  h = MSG_host_self();
  h_simdata = h->simdata;

  DEBUG2("Waiting for a task on channel %d (%s)", channel,h->name);

  while ((t = xbt_fifo_shift(h_simdata->mbox[channel])) == NULL) {
    if(max_duration>0) {
      if(!first_time) {
	MSG_RETURN(MSG_OK);
      }
    }
    xbt_assert2(!(h_simdata->sleeping[channel]),
		"A process (%s(%d)) is already blocked on this channel",
		h_simdata->sleeping[channel]->name,
		h_simdata->sleeping[channel]->simdata->PID);
    h_simdata->sleeping[channel] = process; /* I'm waiting. Wake me up when you're ready */
    if(max_duration>0) {
      __MSG_process_block(max_duration);
    } else {
      __MSG_process_block(-1);
    }
    if(surf_workstation_resource->extension_public->get_state(h_simdata->host) 
       == SURF_CPU_OFF)
      MSG_RETURN(MSG_HOST_FAILURE);
    h_simdata->sleeping[channel] = NULL;
    first_time = 0;
    /* OK, we should both be ready now. Are you there ? */
  }

  t_simdata = t->simdata;
  /*   *task = __MSG_task_copy(t); */
  *task=t;

  /* Transfer */
  t_simdata->using++;

  t_simdata->comm = surf_workstation_resource->extension_public->
    communicate(MSG_process_get_host(t_simdata->sender)->simdata->host,
		h->simdata->host, t_simdata->message_size,t_simdata->rate);
  
  surf_workstation_resource->common_public->action_set_data(t_simdata->comm,t);

  if(__MSG_process_isBlocked(t_simdata->sender)) 
    __MSG_process_unblock(t_simdata->sender);

  PAJE_PROCESS_PUSH_STATE(process,"C");  

  do {
    __MSG_task_wait_event(process, t);
    state=surf_workstation_resource->common_public->action_get_state(t_simdata->comm);
  } while (state==SURF_ACTION_RUNNING);

  if(t->simdata->using>1) {
    xbt_fifo_unshift(msg_global->process_to_run,process);
    xbt_context_yield();
  }

  PAJE_PROCESS_POP_STATE(process);
  PAJE_COMM_STOP(process,t,channel);

  if(state == SURF_ACTION_DONE) {
    if(surf_workstation_resource->common_public->action_free(t_simdata->comm)) 
      t_simdata->comm = NULL;
    MSG_RETURN(MSG_OK);
  } else if(surf_workstation_resource->extension_public->get_state(h_simdata->host) 
	  == SURF_CPU_OFF) {
    if(surf_workstation_resource->common_public->action_free(t_simdata->comm)) 
      t_simdata->comm = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    if(surf_workstation_resource->common_public->action_free(t_simdata->comm)) 
      t_simdata->comm = NULL;
    MSG_RETURN(MSG_TRANSFER_FAILURE);
  }
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

  DEBUG2("Probing on channel %d (%s)", channel,h->name);
  CHECK_HOST();
  h = MSG_host_self();
  h_simdata = h->simdata;
  return(xbt_fifo_getFirstItem(h_simdata->mbox[channel])!=NULL);
}

/** \ingroup msg_gos_functions
 * \brief Test whether there is a pending communication on a channel, and who sent it.
 *
 * It takes one parameter.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \return -1 if there is no pending communication and the PID of the process who sent it otherwise
 */
int MSG_task_probe_from(m_channel_t channel)
{
  m_host_t h = NULL;
  simdata_host_t h_simdata = NULL;
  xbt_fifo_item_t item;
  m_task_t t;

  CHECK_HOST();
  h = MSG_host_self();
  h_simdata = h->simdata;

  DEBUG2("Probing on channel %d (%s)", channel,h->name);
   
  item = xbt_fifo_getFirstItem(h->simdata->mbox[channel]);
  if (!item || !(t = xbt_fifo_get_item_content(item)))
    return -1;
   
  return MSG_process_get_PID(t->simdata->sender);
}

MSG_error_t MSG_channel_select_from(m_channel_t channel, double max_duration,
				    int *PID)
{
  m_host_t h = NULL;
  simdata_host_t h_simdata = NULL;
  xbt_fifo_item_t item;
  m_task_t t;
  int first_time = 1;
  m_process_t process = MSG_process_self();

  if(PID) {
    *PID = -1;
  }

  if(max_duration==0.0) {
    return MSG_task_probe_from(channel);
  } else {
    CHECK_HOST();
    h = MSG_host_self();
    h_simdata = h->simdata;
    
    DEBUG2("Probing on channel %d (%s)", channel,h->name);
    while(!(item = xbt_fifo_getFirstItem(h->simdata->mbox[channel]))) {
      if(max_duration>0) {
	if(!first_time) {
	  MSG_RETURN(MSG_OK);
	}
      }
      xbt_assert2(!(h_simdata->sleeping[channel]),
		  "A process (%s(%d)) is already blocked on this channel",
		  h_simdata->sleeping[channel]->name,
		  h_simdata->sleeping[channel]->simdata->PID);
      h_simdata->sleeping[channel] = process; /* I'm waiting. Wake me up when you're ready */
      if(max_duration>0) {
	__MSG_process_block(max_duration);
      } else {
	__MSG_process_block(-1);
      }
      if(surf_workstation_resource->extension_public->get_state(h_simdata->host) 
	 == SURF_CPU_OFF) {
	MSG_RETURN(MSG_HOST_FAILURE);
      }
      h_simdata->sleeping[channel] = NULL;
      first_time = 0;
    }
    if (!item || !(t = xbt_fifo_get_item_content(item))) {
      MSG_RETURN(MSG_OK);
    }
    if(PID) {
      *PID = MSG_process_get_PID(t->simdata->sender);
    }
    MSG_RETURN(MSG_OK);
  }
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

  DEBUG4("Trying to send a task (%g Mb) from %s to %s on channel %d", 
	 task->simdata->message_size,local_host->name, remote_host->name, channel);

  xbt_fifo_push(((simdata_host_t) remote_host->simdata)->
		mbox[channel], task);

  PAJE_COMM_START(process,task,channel);
    
  if(remote_host->simdata->sleeping[channel]) 
    __MSG_process_unblock(remote_host->simdata->sleeping[channel]);

  process->simdata->put_host = dest;
  process->simdata->put_channel = channel;
  while(!(task_simdata->comm)) 
    __MSG_process_block(-1);
  surf_workstation_resource->common_public->action_use(task_simdata->comm);
  process->simdata->put_host = NULL;
  process->simdata->put_channel = -1;


  PAJE_PROCESS_PUSH_STATE(process,"C");  

  state=surf_workstation_resource->common_public->action_get_state(task_simdata->comm);
  while (state==SURF_ACTION_RUNNING) {
    __MSG_task_wait_event(process, task);
    state=surf_workstation_resource->common_public->action_get_state(task_simdata->comm);
  }
    

  PAJE_PROCESS_POP_STATE(process);  

  if(state == SURF_ACTION_DONE) {
    if(surf_workstation_resource->common_public->action_free(task_simdata->comm)) 
      task_simdata->comm = NULL;
    MSG_task_destroy(task);
    MSG_RETURN(MSG_OK);
  } else if(surf_workstation_resource->extension_public->get_state(local_host->simdata->host) 
	    == SURF_CPU_OFF) {
    if(surf_workstation_resource->common_public->action_free(task_simdata->comm)) 
      task_simdata->comm = NULL;
    MSG_task_destroy(task);
    MSG_RETURN(MSG_HOST_FAILURE);
  } else { 
    if(surf_workstation_resource->common_public->action_free(task_simdata->comm)) 
      task_simdata->comm = NULL;
    MSG_task_destroy(task);
    MSG_RETURN(MSG_TRANSFER_FAILURE);
  }
}

/** \ingroup msg_gos_functions
 * \brief Does exactly the same as MSG_task_put but with a bounded transmition 
 * rate.
 *
 * \sa MSG_task_put
 */
MSG_error_t MSG_task_put_bounded(m_task_t task,
				 m_host_t dest, m_channel_t channel,
				 double max_rate)
{
  MSG_error_t res = MSG_OK;
  task->simdata->rate=max_rate;
  res = MSG_task_put(task, dest, channel);
  task->simdata->rate=-1.0;
  return(res);
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
  MSG_error_t res;

  DEBUG1("Computing on %s", process->simdata->host->name);

  __MSG_task_execute(process, task);

  PAJE_PROCESS_PUSH_STATE(process,"E");  
  res = __MSG_wait_for_computation(process,task);
  PAJE_PROCESS_POP_STATE(process);
  return res;
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
    

  if(state == SURF_ACTION_DONE) {
    if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
      simdata->compute = NULL;
    simdata->computation_amount = 0.0;
    MSG_RETURN(MSG_OK);
  } else if(surf_workstation_resource->extension_public->
	    get_state(MSG_process_get_host(process)->simdata->host) 
	    == SURF_CPU_OFF) {
    if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
      simdata->compute = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
      simdata->compute = NULL;
    MSG_RETURN(MSG_TASK_CANCELLED);
  }
}
/** \ingroup m_task_management
 * \brief Creates a new #m_task_t (a parallel one....).
 *
 * A constructor for #m_task_t taking six arguments and returning the 
   corresponding object.
 * \param name a name for the object. It is for user-level information
   and can be NULL.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of #host_nb m_host_t.
 * \param computation_amount an array of #host_nb
   doubles. computation_amount[i] is the total number of operations
   that have to be performed on host_list[i].
 * \param communication_amount an array of #host_nb*#host_nb doubles.
 * \param data a pointer to any data may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_task_get_data.
 * \see m_task_t
 * \return The new corresponding object.
 */
m_task_t MSG_parallel_task_create(const char *name, 
				  int host_nb,
				  const m_host_t *host_list,
				  double *computation_amount,
				  double *communication_amount,
				  void *data)
{
  simdata_task_t simdata = xbt_new0(s_simdata_task_t,1);
  m_task_t task = xbt_new0(s_m_task_t,1);
  int i;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->simdata = simdata;
  task->data = data;

  /* Simulator Data */
  simdata->sleeping = xbt_dynar_new(sizeof(m_process_t),NULL);
  simdata->rate = -1.0;
  simdata->using = 1;
  simdata->sender = NULL;
  simdata->host_nb = host_nb;
  
  simdata->host_list = xbt_new0(void *, host_nb);
  simdata->comp_amount = computation_amount;
  simdata->comm_amount = communication_amount;

  for(i=0;i<host_nb;i++)
    simdata->host_list[i] = host_list[i]->simdata->host;

  return task;
}


static void __MSG_parallel_task_execute(m_process_t process, m_task_t task)
{
  simdata_task_t simdata = NULL;

  CHECK_HOST();

  simdata = task->simdata;

  xbt_assert0(simdata->host_nb,"This is not a parallel task. Go to hell.");

  simdata->compute = surf_workstation_resource->extension_public->
  execute_parallel_task(task->simdata->host_nb,
			task->simdata->host_list,
			task->simdata->comp_amount,
			task->simdata->comm_amount,
			1.0,
			-1.0);
  if(simdata->compute)
    surf_workstation_resource->common_public->action_set_data(simdata->compute,task);
}

MSG_error_t MSG_parallel_task_execute(m_task_t task)
{
  m_process_t process = MSG_process_self();
  MSG_error_t res;

  DEBUG0("Computing on a tons of guys");
  
  __MSG_parallel_task_execute(process, task);

  if(task->simdata->compute)
    res = __MSG_wait_for_computation(process,task);
  else 
    res = MSG_OK;

  return res;  
}


/** \ingroup msg_gos_functions
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
MSG_error_t MSG_process_sleep(double nb_sec)
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
       == SURF_CPU_OFF) {
      if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
	simdata->compute = NULL;
      MSG_RETURN(MSG_HOST_FAILURE);
    }
    if(__MSG_process_isBlocked(process)) {
      __MSG_process_unblock(MSG_process_self());
    }
    if(surf_workstation_resource->extension_public->
       get_state(MSG_process_get_host(process)->simdata->host) 
       == SURF_CPU_OFF) {
      if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
	simdata->compute = NULL;
      MSG_RETURN(MSG_HOST_FAILURE);
    }
    if(surf_workstation_resource->common_public->action_free(simdata->compute)) 
      simdata->compute = NULL;
    MSG_task_destroy(dummy);
    MSG_RETURN(MSG_OK);
  } else MSG_RETURN(MSG_HOST_FAILURE);
}

/** \ingroup msg_gos_functions
 * \brief Return the number of MSG tasks currently running on a
 * the host of the current running process.
 */
static int MSG_get_msgload(void) 
{
  m_process_t process;
   
  CHECK_HOST();
  
  xbt_assert0(0, "This function is still to be specified correctly (what do you mean by 'load', exactly?). In the meantime, please don't use it");
  process = MSG_process_self();
  return xbt_fifo_size(process->simdata->host->simdata->process_list);
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
