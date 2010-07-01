/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "mailbox.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg,
                                "Logging specific to MSG (gos)");

/** \ingroup msg_gos_functions
 *
 * \brief Return the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
MSG_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
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
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  CHECK_HOST();

  simdata = task->simdata;

  xbt_assert0(simdata->host_nb==0, "This is a parallel task. Go to hell.");

#ifdef HAVE_TRACING
  TRACE_msg_task_execute_start (task);
#endif

  xbt_assert1((!simdata->compute) && (task->simdata->refcount == 1),
              "This task is executed somewhere else. Go fix your code! %d", task->simdata->refcount);

  DEBUG1("Computing on %s", MSG_process_self()->simdata->m_host->name);

  if (simdata->computation_amount == 0) {
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end (task);
#endif
    return MSG_OK;
  }
  simdata->refcount++;
  SIMIX_mutex_lock(simdata->mutex);
  simdata->compute =
    SIMIX_action_execute(SIMIX_host_self(), task->name,
                         simdata->computation_amount);
  SIMIX_action_set_priority(simdata->compute, simdata->priority);

  /* changed to waiting action since we are always waiting one action (execute, communicate or sleep) */
  self->simdata->waiting_action = simdata->compute;
  SIMIX_register_action_to_condition(simdata->compute, simdata->cond);
  do {
    SIMIX_cond_wait(simdata->cond, simdata->mutex);
    state = SIMIX_action_get_state(simdata->compute);
  } while (state == SURF_ACTION_READY || state == SURF_ACTION_RUNNING);
  SIMIX_unregister_action_to_condition(simdata->compute, simdata->cond);
  self->simdata->waiting_action = NULL;

  SIMIX_mutex_unlock(simdata->mutex);
  simdata->refcount--;

  if (SIMIX_action_get_state(task->simdata->compute) == SURF_ACTION_DONE) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    SIMIX_action_destroy(task->simdata->compute);
    simdata->computation_amount = 0.0;
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end (task);
#endif
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_host_get_state(SIMIX_host_self()) == 0) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    SIMIX_action_destroy(task->simdata->compute);
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end (task);
#endif
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    SIMIX_action_destroy(task->simdata->compute);
    simdata->comm = NULL;
    simdata->compute = NULL;
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end (task);
#endif
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
m_task_t
MSG_parallel_task_create(const char *name, int host_nb,
                         const m_host_t * host_list,
                         double *computation_amount,
                         double *communication_amount, void *data)
{
  int i;
  simdata_task_t simdata = xbt_new0(s_simdata_task_t, 1);
  m_task_t task = xbt_new0(s_m_task_t, 1);
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
  simdata->refcount = 1;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->source = NULL;

  simdata->host_nb = host_nb;
  simdata->host_list = xbt_new0(smx_host_t, host_nb);
  simdata->comp_amount = computation_amount;
  simdata->comm_amount = communication_amount;

  for (i = 0; i < host_nb; i++)
    simdata->host_list[i] = host_list[i]->simdata->smx_host;

  return task;
}

MSG_error_t MSG_parallel_task_execute(m_task_t task)
{
  simdata_task_t simdata = NULL;
  m_process_t self = MSG_process_self();
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  CHECK_HOST();

  simdata = task->simdata;

  xbt_assert0((!simdata->compute)
              && (task->simdata->refcount == 1),
              "This task is executed somewhere else. Go fix your code!");

  xbt_assert0(simdata->host_nb, "This is not a parallel task. Go to hell.");

  DEBUG1("Computing on %s", MSG_process_self()->simdata->m_host->name);

  simdata->refcount++;

  SIMIX_mutex_lock(simdata->mutex);
  simdata->compute =
    SIMIX_action_parallel_execute(task->name, simdata->host_nb,
                                  simdata->host_list, simdata->comp_amount,
                                  simdata->comm_amount, 1.0, -1.0);

  self->simdata->waiting_action = simdata->compute;
  SIMIX_register_action_to_condition(simdata->compute, simdata->cond);
  do {
    SIMIX_cond_wait(simdata->cond, simdata->mutex);
    state = SIMIX_action_get_state(task->simdata->compute);
  } while (state == SURF_ACTION_READY || state == SURF_ACTION_RUNNING);

  SIMIX_unregister_action_to_condition(simdata->compute, simdata->cond);
  self->simdata->waiting_action = NULL;


  SIMIX_mutex_unlock(simdata->mutex);
  simdata->refcount--;

  if (SIMIX_action_get_state(task->simdata->compute) == SURF_ACTION_DONE) {
    /* action ended, set comm and compute = NULL, the actions is already destroyed in the main function */
    SIMIX_action_destroy(task->simdata->compute);
    simdata->computation_amount = 0.0;
    simdata->comm = NULL;
    simdata->compute = NULL;
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_host_get_state(SIMIX_host_self()) == 0) {
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
  e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
  smx_mutex_t mutex;
  smx_cond_t cond;

#ifdef HAVE_TRACING
  TRACE_msg_process_sleep_in (MSG_process_self());
#endif

  /* create action to sleep */
  act_sleep =
    SIMIX_action_sleep(SIMIX_process_get_host(proc->simdata->s_process),
                       nb_sec);

  mutex = SIMIX_mutex_init();
  SIMIX_mutex_lock(mutex);

  /* create conditional and register action to it */
  cond = SIMIX_cond_init();

  proc->simdata->waiting_action = act_sleep;
  SIMIX_register_action_to_condition(act_sleep, cond);
  do {
    SIMIX_cond_wait(cond, mutex);
    state = SIMIX_action_get_state(act_sleep);
  } while (state == SURF_ACTION_READY || state == SURF_ACTION_RUNNING);
  proc->simdata->waiting_action = NULL;
  SIMIX_unregister_action_to_condition(act_sleep, cond);
  SIMIX_mutex_unlock(mutex);

  /* remove variables */
  SIMIX_cond_destroy(cond);
  SIMIX_mutex_destroy(mutex);

  if (SIMIX_action_get_state(act_sleep) == SURF_ACTION_DONE) {
    if (SIMIX_host_get_state(SIMIX_host_self()) == SURF_RESOURCE_OFF) {
      SIMIX_action_destroy(act_sleep);
#ifdef HAVE_TRACING
      TRACE_msg_process_sleep_out (MSG_process_self());
#endif
      MSG_RETURN(MSG_HOST_FAILURE);
    }
  } else {
    SIMIX_action_destroy(act_sleep);
#ifdef HAVE_TRACING
    TRACE_msg_process_sleep_out (MSG_process_self());
#endif
    MSG_RETURN(MSG_HOST_FAILURE);
  }

  SIMIX_action_destroy(act_sleep);
#ifdef HAVE_TRACING
  TRACE_msg_process_sleep_out (MSG_process_self());
#endif
  MSG_RETURN(MSG_OK);
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
MSG_error_t
MSG_task_get_from_host(m_task_t * task, m_channel_t channel, m_host_t host)
{
  return MSG_task_get_ext(task, channel, -1, host);
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
MSG_error_t MSG_task_get(m_task_t * task, m_channel_t channel)
{
  return MSG_task_get_with_timeout(task, channel, -1);
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
MSG_error_t
MSG_task_get_with_timeout(m_task_t * task, m_channel_t channel,
                          double max_duration)
{
  return MSG_task_get_ext(task, channel, max_duration, NULL);
}

/** \defgroup msg_gos_functions MSG Operating System Functions
 *  \brief This section describes the functions that can be used
 *  by an agent for handling some task.
 */

MSG_error_t
MSG_task_get_ext(m_task_t * task, m_channel_t channel, double timeout,
                 m_host_t host)
{
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
    MSG_mailbox_get_task_ext(MSG_mailbox_get_by_channel
                             (MSG_host_self(), channel), task, host, timeout);
}

MSG_error_t
MSG_task_receive_from_host(m_task_t * task, const char *alias, m_host_t host)
{
  return MSG_task_receive_ext(task, alias, -1, host);
}

MSG_error_t MSG_task_receive(m_task_t * task, const char *alias)
{
  return MSG_task_receive_with_timeout(task, alias, -1);
}

MSG_error_t
MSG_task_receive_with_timeout(m_task_t * task, const char *alias,
                              double timeout)
{
  return MSG_task_receive_ext(task, alias, timeout, NULL);
}

MSG_error_t
MSG_task_receive_ext(m_task_t * task, const char *alias, double timeout,
                     m_host_t host)
{
  return MSG_mailbox_get_task_ext(MSG_mailbox_get_by_alias(alias), task, host,
                                  timeout);
}

msg_comm_t MSG_task_isend(m_task_t task, const char *alias) {
  simdata_task_t t_simdata = NULL;
  m_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  CHECK_HOST();

  /* FIXME: these functions are not tracable */

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  xbt_assert0(t_simdata->refcount == 1,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->refcount++;
  msg_global->sent_msg++;
  process->simdata->waiting_task = task;

  /* Send it by calling SIMIX network layer */

  /* Kept for semantical compatibility with older implementation */
  if(mailbox->cond)
    SIMIX_cond_signal(mailbox->cond);

  return SIMIX_network_isend(mailbox->rdv, t_simdata->message_size, t_simdata->rate,
      task, sizeof(void*), &t_simdata->comm);
}

msg_comm_t MSG_task_irecv(m_task_t * task, const char *alias) {
  smx_comm_t comm;
  smx_rdv_t rdv = MSG_mailbox_get_by_alias(alias)->rdv;
  msg_mailbox_t mailbox=MSG_mailbox_get_by_alias(alias);
  size_t size = sizeof(void*);

  CHECK_HOST();

  /* FIXME: these functions are not tracable */

  memset(&comm,0,sizeof(comm));

  /* Kept for compatibility with older implementation */
  xbt_assert1(!MSG_mailbox_get_cond(mailbox),
              "A process is already blocked on this channel %s",
              MSG_mailbox_get_alias(mailbox));

  /* Sanity check */
  xbt_assert0(task, "Null pointer for the task storage");

  if (*task)
    CRITICAL0("MSG_task_get() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  return SIMIX_network_irecv(rdv, task, &size);
}
int MSG_comm_test(msg_comm_t comm) {
  return SIMIX_network_test(comm);
}

MSG_error_t MSG_comm_wait(msg_comm_t comm, double timeout) {
  xbt_ex_t e;
  MSG_error_t res = MSG_OK;
  TRY {
    SIMIX_network_wait(comm,timeout);

		if(!(comm->src_proc == SIMIX_process_self()))
    {
    	m_task_t  task;
    	task = (m_task_t) SIMIX_communication_get_src_buf(comm);
    	task->simdata->refcount--;
    }

    /* FIXME: these functions are not tracable */
  }  CATCH(e){
      switch(e.category){
        case host_error:
          res = MSG_HOST_FAILURE;
          break;
        case network_error:
          res = MSG_TRANSFER_FAILURE;
          break;
        case timeout_error:
          res = MSG_TIMEOUT;
          break;
        default:
          xbt_die(bprintf("Unhandled SIMIX network exception: %s",e.msg));
      }
      xbt_ex_free(e);
    }
  return res;
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
MSG_error_t MSG_task_put(m_task_t task, m_host_t dest, m_channel_t channel)
{
  return MSG_task_put_with_timeout(task, dest, channel, -1.0);
}

/** \ingroup msg_gos_functions
 * \brief Does exactly the same as MSG_task_put but with a bounded transmition
 * rate.
 *
 * \sa MSG_task_put
 */
MSG_error_t
MSG_task_put_bounded(m_task_t task, m_host_t dest, m_channel_t channel,
                     double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_put(task, dest, channel);
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
 * \param timeout the maximum time to wait for a task before giving
    up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task
    will not be modified
 * \return #MSG_FATAL if \a task is not properly initialized and
   #MSG_OK otherwise. Returns #MSG_HOST_FAILURE if the host on which
   this function was called was shut down. Returns
   #MSG_TRANSFER_FAILURE if the transfer could not be properly done
   (network failure, dest failure, timeout...)
 */
MSG_error_t
MSG_task_put_with_timeout(m_task_t task, m_host_t dest, m_channel_t channel,
                          double timeout)
{
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
    MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_channel(dest, channel),
                                 task, timeout);
}

MSG_error_t MSG_task_send(m_task_t task, const char *alias)
{
  return MSG_task_send_with_timeout(task, alias, -1);
}


MSG_error_t
MSG_task_send_bounded(m_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_send(task, alias);
}


MSG_error_t
MSG_task_send_with_timeout(m_task_t task, const char *alias, double timeout)
{
  return MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_alias(alias), task,
                                      timeout);
}

int MSG_task_listen(const char *alias)
{
  CHECK_HOST();

  return !MSG_mailbox_is_empty(MSG_mailbox_get_by_alias(alias));
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
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  CHECK_HOST();

  return
    !MSG_mailbox_is_empty(MSG_mailbox_get_by_channel
                          (MSG_host_self(), channel));
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
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  CHECK_HOST();

  return
    MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_channel
                                             (MSG_host_self(), channel),
                                             host);

}

int MSG_task_listen_from_host(const char *alias, m_host_t host)
{
  CHECK_HOST();

  return
    MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_alias(alias),
                                             host);
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
  m_task_t task;

  CHECK_HOST();

  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  if (NULL ==
      (task =
       MSG_mailbox_get_head(MSG_mailbox_get_by_channel
                            (MSG_host_self(), channel))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}

int MSG_task_listen_from(const char *alias)
{
  m_task_t task;

  CHECK_HOST();

  if (NULL == (task = MSG_mailbox_get_head(MSG_mailbox_get_by_alias(alias))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}

/** \ingroup msg_gos_functions
 * \brief Wait for at most \a max_duration second for a task reception
   on \a channel.

 * \a PID is updated with the PID of the first process that triggered this event if any.
 *
 * It takes three parameters:
 * \param channel the channel on which the agent should be
   listening. This value has to be >=0 and < than the maximal.
   number of channels fixed with MSG_set_channel_number().
 * \param PID a memory location for storing an int.
 * \param timeout the maximum time to wait for a task before
    giving up. In the case of a reception, *\a PID will be updated
    with the PID of the first process to send a task.
 * \return #MSG_HOST_FAILURE if the host is shut down in the meantime
   and #MSG_OK otherwise.
 */
MSG_error_t
MSG_channel_select_from(m_channel_t channel, double timeout, int *PID)
{
  m_host_t h = NULL;
  simdata_host_t h_simdata = NULL;
  m_task_t t;
  int first_time = 1;
  smx_cond_t cond;
  msg_mailbox_t mailbox;

  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  if (PID) {
    *PID = -1;
  }

  if (timeout == 0.0) {
    *PID = MSG_task_probe_from(channel);
    MSG_RETURN(MSG_OK);
  } else {
    CHECK_HOST();
    h = MSG_host_self();
    h_simdata = h->simdata;

    mailbox = MSG_mailbox_get_by_channel(MSG_host_self(), channel);

    while (MSG_mailbox_is_empty(mailbox)) {
      if (timeout > 0) {
        if (!first_time) {
          MSG_RETURN(MSG_OK);
        }
      }

      SIMIX_mutex_lock(h_simdata->mutex);

      xbt_assert1(!MSG_mailbox_get_cond(mailbox),
                  "A process is already blocked on this channel %d", channel);

      cond = SIMIX_cond_init();

      MSG_mailbox_set_cond(mailbox, cond);

      if (timeout > 0) {
        SIMIX_cond_wait_timeout(cond, h_simdata->mutex, timeout);
      } else {
        SIMIX_cond_wait(cond, h_simdata->mutex);
      }

      SIMIX_cond_destroy(cond);
      SIMIX_mutex_unlock(h_simdata->mutex);

      if (SIMIX_host_get_state(h_simdata->smx_host) == 0) {
        MSG_RETURN(MSG_HOST_FAILURE);
      }

      MSG_mailbox_set_cond(mailbox, NULL);
      first_time = 0;
    }

    if (NULL == (t = MSG_mailbox_get_head(mailbox)))
      MSG_RETURN(MSG_OK);


    if (PID) {
      *PID = MSG_process_get_PID(t->simdata->sender);
    }

    MSG_RETURN(MSG_OK);
  }
}


MSG_error_t MSG_alias_select_from(const char *alias, double timeout, int *PID)
{
  m_host_t h = NULL;
  simdata_host_t h_simdata = NULL;
  m_task_t t;
  int first_time = 1;
  smx_cond_t cond;
  msg_mailbox_t mailbox;

  if (PID) {
    *PID = -1;
  }

  if (timeout == 0.0) {
    *PID = MSG_task_listen_from(alias);
    MSG_RETURN(MSG_OK);
  } else {
    CHECK_HOST();
    h = MSG_host_self();
    h_simdata = h->simdata;

    DEBUG2("Probing on alias %s (%s)", alias, h->name);

    mailbox = MSG_mailbox_get_by_alias(alias);

    while (MSG_mailbox_is_empty(mailbox)) {
      if (timeout > 0) {
        if (!first_time) {
          MSG_RETURN(MSG_OK);
        }
      }

      SIMIX_mutex_lock(h_simdata->mutex);

      xbt_assert1(!MSG_mailbox_get_cond(mailbox),
                  "A process is already blocked on this alias %s", alias);

      cond = SIMIX_cond_init();

      MSG_mailbox_set_cond(mailbox, cond);

      if (timeout > 0) {
        SIMIX_cond_wait_timeout(cond, h_simdata->mutex, timeout);
      } else {
        SIMIX_cond_wait(cond, h_simdata->mutex);
      }

      SIMIX_cond_destroy(cond);
      SIMIX_mutex_unlock(h_simdata->mutex);

      if (SIMIX_host_get_state(h_simdata->smx_host) == 0) {
        MSG_RETURN(MSG_HOST_FAILURE);
      }

      MSG_mailbox_set_cond(mailbox, NULL);
      first_time = 0;
    }

    if (NULL == (t = MSG_mailbox_get_head(mailbox)))
      MSG_RETURN(MSG_OK);


    if (PID) {
      *PID = MSG_process_get_PID(t->simdata->sender);
    }

    MSG_RETURN(MSG_OK);
  }
}
