/*     $Id$      */
  
/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
  
#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg, "Logging specific to MSG (gos)");

/** \defgroup msg_gos_functions MSG Operating System Functions
 *  \brief This section describes the functions that can be used
 *  by an agent for handling some task.
 */

static MSG_error_t __MSG_task_get_with_time_out_from_host(m_task_t * task,
							m_channel_t channel,
							double max_duration,
							m_host_t host)
{

  m_process_t process = MSG_process_self();
  m_task_t t = NULL;
  m_host_t h = NULL;
  simdata_task_t t_simdata = NULL;
  simdata_host_t h_simdata = NULL;
  int first_time = 1;
  xbt_fifo_item_t item = NULL;

	smx_cond_t cond = NULL;			//conditional wait if the task isn't on the channel yet

  CHECK_HOST();
  xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);
  /* Sanity check */
  xbt_assert0(task,"Null pointer for the task\n");

  if (*task) 
    CRITICAL0("MSG_task_get() was asked to write in a non empty task struct.");

  /* Get the task */
  h = MSG_host_self();
  h_simdata = h->simdata;

  DEBUG2("Waiting for a task on channel %d (%s)", channel,h->name);

	SIMIX_mutex_lock(h->simdata->mutex);
  while (1) {
		if(xbt_fifo_size(h_simdata->mbox[channel])>0) {
			if(!host) {
				t = xbt_fifo_shift(h_simdata->mbox[channel]);
				break;
			} else {
				xbt_fifo_foreach(h->simdata->mbox[channel],item,t,m_task_t) {
					if(t->simdata->source==host) break;
				}
				if(item) {
					xbt_fifo_remove_item(h->simdata->mbox[channel],item);
					break;
				} 
			}
		}
		
		if(max_duration>0) {
			if(!first_time) {
				SIMIX_mutex_unlock(h->simdata->mutex);
				h_simdata->sleeping[channel] = NULL;
				SIMIX_cond_destroy(cond);
				MSG_RETURN(MSG_TRANSFER_FAILURE);
			}
		}
		xbt_assert1(!(h_simdata->sleeping[channel]),"A process is already blocked on channel %d", channel);
	
		cond = SIMIX_cond_init();
		h_simdata->sleeping[channel] = cond;
		if (max_duration > 0) {
			SIMIX_cond_wait_timeout(cond, h->simdata->mutex, max_duration);
		}
		else SIMIX_cond_wait(h_simdata->sleeping[channel],h->simdata->mutex);

		if(SIMIX_host_get_state(h_simdata->host)==0)
      MSG_RETURN(MSG_HOST_FAILURE);

		first_time = 0;
	}
	SIMIX_mutex_unlock(h->simdata->mutex);

  DEBUG1("OK, got a task (%s)", t->name);
	/* clean conditional */
	if (cond) {
		SIMIX_cond_destroy(cond);
		h_simdata->sleeping[channel] = NULL;
	}

  t_simdata = t->simdata;
	t_simdata->receiver = process;
  *task=t;

	SIMIX_mutex_lock(t_simdata->mutex);

  /* Transfer */
  t_simdata->using++;
	/* create SIMIX action to the communication */
	t_simdata->comm = SIMIX_action_communicate(t_simdata->sender->simdata->host->simdata->host,
																						process->simdata->host->simdata->host,t->name, t_simdata->message_size, 
																						t_simdata->rate); 
	/* if the process is suspend, create the action but stop its execution, it will be restart when the sender process resume */
	if (MSG_process_is_suspended(t_simdata->sender)) {
		DEBUG1("Process sender (%s) suspended", t_simdata->sender->name);
		SIMIX_action_set_priority(t_simdata->comm,0);
	}
	process->simdata->waiting_task = t;
	SIMIX_register_action_to_condition(t_simdata->comm, t_simdata->cond);
	SIMIX_register_condition_to_action(t_simdata->comm, t_simdata->cond);
	SIMIX_cond_wait(t_simdata->cond,t_simdata->mutex);
	process->simdata->waiting_task = NULL;

	/* the task has already finished and the pointer must be null*/
	if (t->simdata->sender) {
		t->simdata->sender->simdata->waiting_task = NULL;
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		//t->simdata->comm = NULL;
		//t->simdata->compute = NULL;
	}
	/* for this process, don't need to change in get function*/
	t->simdata->receiver = NULL;
	SIMIX_mutex_unlock(t_simdata->mutex);


	if(SIMIX_action_get_state(t_simdata->comm) == SURF_ACTION_DONE) {
		//t_simdata->comm = NULL;
		SIMIX_action_destroy(t_simdata->comm);
		t_simdata->comm = NULL;
    MSG_RETURN(MSG_OK);
	} else if (SIMIX_host_get_state(h_simdata->host)==0) {
		//t_simdata->comm = NULL;
		SIMIX_action_destroy(t_simdata->comm);
		t_simdata->comm = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else { 
		//t_simdata->comm = NULL;
		SIMIX_action_destroy(t_simdata->comm);
		t_simdata->comm = NULL;
    MSG_RETURN(MSG_TRANSFER_FAILURE);
  }

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
    up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task 
    will not be modified and will still be
    equal to \c NULL when returning. 
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
   if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get_with_time_out(m_task_t * task,
				       m_channel_t channel,
				       double max_duration)
{
  return __MSG_task_get_with_time_out_from_host(task, channel, max_duration, NULL);
}

/** \ingroup msg_gos_functions
 * \brief Listen on \a channel and waits for receiving a task from \a host.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #m_task_t. It will
   hold a task when this function will return. Thus \a task should not
   be equal to \c NULL and \a *task should be equal to \c NULL. If one of
   those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return #MSG_FATAL if \a task is equal to \c NULL, #MSG_WARNING
   if \a *task is not equal to \c NULL, and #MSG_OK otherwise.
 */
MSG_error_t MSG_task_get_from_host(m_task_t * task, int channel, 
				   m_host_t host)
{
  return __MSG_task_get_with_time_out_from_host(task, channel, -1, host);
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

  xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);
  CHECK_HOST();

  DEBUG2("Probing on channel %d (%s)", channel,h->name);

  h = MSG_host_self();
  return(xbt_fifo_get_first_item(h->simdata->mbox[channel])!=NULL);
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
  xbt_fifo_item_t item;
  m_task_t t;

  xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);
  CHECK_HOST();

  h = MSG_host_self();

  DEBUG2("Probing on channel %d (%s)", channel,h->name);
   
  item = xbt_fifo_get_first_item(h->simdata->mbox[channel]);
  if ( (!item) || (!(t = xbt_fifo_get_item_content(item))) )
    return -1;
   
  return MSG_process_get_PID(t->simdata->sender);
}

/** \ingroup msg_gos_functions
 * \brief Wait for at most \a max_duration second for a task reception
   on \a channel. *\a PID is updated with the PID of the first process
   that triggered this event if any.
 *
 * It takes three parameters:
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal.
   number of channels fixed with MSG_set_channel_number().
 * \param PID a memory location for storing an int.
 * \param max_duration the maximum time to wait for a task before
    giving up. In the case of a reception, *\a PID will be updated
    with the PID of the first process to send a task.
 * \return #MSG_HOST_FAILURE if the host is shut down in the meantime
   and #MSG_OK otherwise.
 */
MSG_error_t MSG_channel_select_from(m_channel_t channel, double max_duration,
		int *PID)
{
	m_host_t h = NULL;
	simdata_host_t h_simdata = NULL;
	xbt_fifo_item_t item;
	m_task_t t;
	int first_time = 1;
	smx_cond_t cond;

	xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);
	if(PID) {
		*PID = -1;
	}

	if(max_duration==0.0) {
		*PID = MSG_task_probe_from(channel);
		MSG_RETURN(MSG_OK);
	} else {
		CHECK_HOST();
		h = MSG_host_self();
		h_simdata = h->simdata;

		DEBUG2("Probing on channel %d (%s)", channel,h->name);
		while(!(item = xbt_fifo_get_first_item(h->simdata->mbox[channel]))) {
			if(max_duration>0) {
				if(!first_time) {
					MSG_RETURN(MSG_OK);
				}
			}
			SIMIX_mutex_lock(h_simdata->mutex);
			xbt_assert1(!(h_simdata->sleeping[channel]),
					"A process is already blocked on this channel %d", channel);
			cond = SIMIX_cond_init();
			h_simdata->sleeping[channel] = cond; /* I'm waiting. Wake me up when you're ready */
			if(max_duration>0) {
				SIMIX_cond_wait_timeout(cond,h_simdata->mutex, max_duration);
			} else {
				SIMIX_cond_wait(cond,h_simdata->mutex);
			}
			SIMIX_cond_destroy(cond);
			SIMIX_mutex_unlock(h_simdata->mutex);
			if(SIMIX_host_get_state(h_simdata->host)==0) {
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

 * \brief Return the number of tasks waiting to be received on a \a
   channel and sent by \a host.
 *
 * It takes two parameters.
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal
   number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return the number of tasks waiting to be received on \a channel
   and sent by \a host.
 */
int MSG_task_probe_from_host(int channel, m_host_t host)
{
  xbt_fifo_item_t item;
  m_task_t t;
  int count = 0;
  m_host_t h = NULL;
  
  xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);
  CHECK_HOST();
  h = MSG_host_self();

  DEBUG2("Probing on channel %d (%s)", channel,h->name);
   
  xbt_fifo_foreach(h->simdata->mbox[channel],item,t,m_task_t) {
    if(t->simdata->source==host) count++;
  }
   
  return count;
}

/** \ingroup msg_gos_functions \brief Put a task on a channel of an
 * host (with a timeout on the waiting of the destination host) and
 * waits for the end of the transmission.
 *
 * This function is used for describing the behavior of an agent. It
 * takes four parameter.
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
 * \param max_duration the maximum time to wait for a task before giving
    up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task 
    will not be modified 
 * \return #MSG_FATAL if \a task is not properly initialized and
   #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
   this function was called was shut down. Returns
   #MSG_TRANSFER_FAILURE if the transfer could not be properly done
   (network failure, dest failure, timeout...)
 */
MSG_error_t MSG_task_put_with_timeout(m_task_t task, m_host_t dest, 
				      m_channel_t channel, double max_duration)
{


  m_process_t process = MSG_process_self();
  simdata_task_t task_simdata = NULL;
  m_host_t local_host = NULL;
  m_host_t remote_host = NULL;
  CHECK_HOST();

  xbt_assert1((channel>=0) && (channel < msg_global->max_channel),"Invalid channel %d",channel);

  task_simdata = task->simdata;
  task_simdata->sender = process;
  task_simdata->source = MSG_process_get_host(process);
  xbt_assert0(task_simdata->using==1,
	      "This task is still being used somewhere else. You cannot send it now. Go fix your code!");
  task_simdata->comm = NULL;
  
  local_host = ((simdata_process_t) process->simdata)->host;
  remote_host = dest;

  DEBUG4("Trying to send a task (%g kB) from %s to %s on channel %d", 
	 task->simdata->message_size/1000,local_host->name, remote_host->name, channel);

	SIMIX_mutex_lock(remote_host->simdata->mutex);
  xbt_fifo_push(((simdata_host_t) remote_host->simdata)->
		mbox[channel], task);

  
  if(remote_host->simdata->sleeping[channel]) {
    DEBUG0("Somebody is listening. Let's wake him up!");
		SIMIX_cond_signal(remote_host->simdata->sleeping[channel]);
  }
	SIMIX_mutex_unlock(remote_host->simdata->mutex);

  process->simdata->put_host = dest;
  process->simdata->put_channel = channel;
	SIMIX_mutex_lock(task->simdata->mutex);
 // DEBUG4("Task sent (%g kB) from %s to %s on channel %d, waiting...", task->simdata->message_size/1000,local_host->name, remote_host->name, channel);

	process->simdata->waiting_task = task;
	if (max_duration >0) {
		SIMIX_cond_wait_timeout(task->simdata->cond,task->simdata->mutex,max_duration);
		/* verify if the timeout happened and the communication didn't started yet */
		if (task->simdata->comm==NULL) {
			task->simdata->using--;
			process->simdata->waiting_task = NULL;
			xbt_fifo_remove(((simdata_host_t) remote_host->simdata)->mbox[channel],
			task);
			if (task->simdata->receiver) {
				task->simdata->receiver->simdata->waiting_task = NULL;
			}
			task->simdata->sender = NULL;
			SIMIX_mutex_unlock(task->simdata->mutex);
			MSG_RETURN(MSG_TRANSFER_FAILURE);
		}
	}
	else {
		SIMIX_cond_wait(task->simdata->cond,task->simdata->mutex);
	}

	DEBUG1("Action terminated %s",task->name);    
	task->simdata->using--;
	process->simdata->waiting_task = NULL;
	/* the task has already finished and the pointer must be null*/
	if (task->simdata->receiver) {
		task->simdata->receiver->simdata->waiting_task = NULL;
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
	//	task->simdata->comm = NULL;
		//task->simdata->compute = NULL;
	}
	task->simdata->sender = NULL;
	SIMIX_mutex_unlock(task->simdata->mutex);

	if(SIMIX_action_get_state(task->simdata->comm) == SURF_ACTION_DONE) {
    MSG_RETURN(MSG_OK);
	} else if (SIMIX_host_get_state(local_host->simdata->host)==0) {
    MSG_RETURN(MSG_HOST_FAILURE);
  } else { 
    MSG_RETURN(MSG_TRANSFER_FAILURE);
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
 * #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
 * this function was called was shut down. Returns
 * #MSG_TRANSFER_FAILURE if the transfer could not be properly done
 * (network failure, dest failure)
 */
MSG_error_t MSG_task_put(m_task_t task,
			 m_host_t dest, m_channel_t channel)
{
  return MSG_task_put_with_timeout(task, dest, channel, -1.0);
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
	simdata_task_t simdata = NULL;
	m_process_t self = MSG_process_self();
  CHECK_HOST();

  simdata = task->simdata;
  xbt_assert0((!simdata->compute)&&(task->simdata->using==1),
	      "This task is executed somewhere else. Go fix your code!");
	
	DEBUG1("Computing on %s", MSG_process_self()->simdata->host->name);
  simdata->using++;
	SIMIX_mutex_lock(simdata->mutex);
  simdata->compute = SIMIX_action_execute(SIMIX_host_self(), task->name, simdata->computation_amount);
	SIMIX_action_set_priority(simdata->compute, simdata->priority);

	self->simdata->waiting_task = task;
	SIMIX_register_action_to_condition(simdata->compute, simdata->cond);
	SIMIX_register_condition_to_action(simdata->compute, simdata->cond);
	SIMIX_cond_wait(simdata->cond, simdata->mutex);
	self->simdata->waiting_task = NULL;

	SIMIX_mutex_unlock(simdata->mutex);
  simdata->using--;

	if(SIMIX_action_get_state(task->simdata->compute) == SURF_ACTION_DONE) {
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->computation_amount = 0.0;
		simdata->comm = NULL;
		simdata->compute = NULL;
    MSG_RETURN(MSG_OK);
	} else if (SIMIX_host_get_state(SIMIX_host_self())==0) {
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->comm = NULL;
		simdata->compute = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else { 
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->comm = NULL;
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
 * \param host_list an array of \p host_nb m_host_t.
 * \param computation_amount an array of \p host_nb
   doubles. computation_amount[i] is the total number of operations
   that have to be performed on host_list[i].
 * \param communication_amount an array of \p host_nb* \p host_nb doubles.
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
  int i;
  simdata_task_t simdata = xbt_new0(s_simdata_task_t,1);
  m_task_t task = xbt_new0(s_m_task_t,1);
  task->simdata = simdata;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->computation_amount = 0;
  simdata->message_size = 0;
	simdata->cond = SIMIX_cond_init();
	simdata->mutex = SIMIX_mutex_init();
	simdata->compute = NULL;
	simdata->comm = NULL;
  simdata->rate = -1.0;
  simdata->using = 1;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->source = NULL;

  simdata->host_nb = host_nb;
  simdata->host_list = xbt_new0(void *, host_nb);
  simdata->comp_amount = computation_amount;
  simdata->comm_amount = communication_amount;

  for(i=0;i<host_nb;i++)
    simdata->host_list[i] = host_list[i]->simdata->host;

  return task;

}


MSG_error_t MSG_parallel_task_execute(m_task_t task)
{
	simdata_task_t simdata = NULL;
	m_process_t self = MSG_process_self();
  CHECK_HOST();

  simdata = task->simdata;
  xbt_assert0((!simdata->compute)&&(task->simdata->using==1),
	      "This task is executed somewhere else. Go fix your code!");

  xbt_assert0(simdata->host_nb,"This is not a parallel task. Go to hell.");
	
	DEBUG1("Computing on %s", MSG_process_self()->simdata->host->name);
  simdata->using++;
	SIMIX_mutex_lock(simdata->mutex);
  simdata->compute = SIMIX_action_parallel_execute(task->name, simdata->host_nb, simdata->host_list, simdata->comp_amount, simdata->comm_amount, 1.0, -1.0);

	self->simdata->waiting_task = task;
	SIMIX_register_action_to_condition(simdata->compute, simdata->cond);
	SIMIX_register_condition_to_action(simdata->compute, simdata->cond);
	SIMIX_cond_wait(simdata->cond, simdata->mutex);
	self->simdata->waiting_task = NULL;


	SIMIX_mutex_unlock(simdata->mutex);
  simdata->using--;

	if(SIMIX_action_get_state(task->simdata->compute) == SURF_ACTION_DONE) {
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->computation_amount = 0.0;
		simdata->comm = NULL;
		simdata->compute = NULL;
    MSG_RETURN(MSG_OK);
	} else if (SIMIX_host_get_state(SIMIX_host_self())==0) {
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->comm = NULL;
		simdata->compute = NULL;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else { 
		/* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
		SIMIX_action_destroy(task->simdata->compute);
		simdata->comm = NULL;
		simdata->compute = NULL;
    MSG_RETURN(MSG_TASK_CANCELLED);
  }	

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
	smx_action_t act_sleep;
	m_process_t proc = MSG_process_self();
	smx_mutex_t mutex;
	smx_cond_t cond;
	/* create action to sleep */
	act_sleep = SIMIX_action_sleep(SIMIX_process_get_host(proc->simdata->smx_process),nb_sec);
	
	mutex = SIMIX_mutex_init();
	SIMIX_mutex_lock(mutex);
	/* create conditional and register action to it */
	cond = SIMIX_cond_init();

	SIMIX_register_condition_to_action(act_sleep, cond);
	SIMIX_register_action_to_condition(act_sleep, cond);
	SIMIX_cond_wait(cond,mutex);
	SIMIX_mutex_unlock(mutex);

	/* remove variables */
	SIMIX_cond_destroy(cond);
	SIMIX_mutex_destroy(mutex);

  if(SIMIX_action_get_state(act_sleep) == SURF_ACTION_DONE) {
    if(SIMIX_host_get_state(SIMIX_host_self()) == SURF_CPU_OFF) {
			SIMIX_action_destroy(act_sleep);
      MSG_RETURN(MSG_HOST_FAILURE);
    }
  }
	else {
		SIMIX_action_destroy(act_sleep);
		MSG_RETURN(MSG_HOST_FAILURE);
	}


	MSG_RETURN(MSG_OK);
}

/** \ingroup msg_gos_functions
 * \brief Return the number of MSG tasks currently running on
 * the host of the current running process.
 */
static int MSG_get_msgload(void) 
{
	xbt_die("not implemented yet");
	return 0;
}

/** \ingroup msg_gos_functions
 *
 * \brief Return the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
MSG_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
}
