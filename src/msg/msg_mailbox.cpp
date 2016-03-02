/* Mailboxes in MSG */

/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "msg_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_mailbox, msg, "Logging specific to MSG (mailbox)");

msg_mailbox_t MSG_mailbox_new(const char *alias)
{
  return simcall_rdv_create(alias);
}

void MSG_mailbox_free(void *mailbox)
{
  simcall_rdv_destroy((msg_mailbox_t)mailbox);
}

int MSG_mailbox_is_empty(msg_mailbox_t mailbox)
{
  return (NULL == simcall_rdv_get_head(mailbox));
}

msg_task_t MSG_mailbox_get_head(msg_mailbox_t mailbox)
{
  smx_synchro_t comm = simcall_rdv_get_head(mailbox);

  if (!comm)
    return NULL;

  return (msg_task_t) simcall_comm_get_src_data(comm);
}

int MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox, msg_host_t host)
{
  return simcall_rdv_comm_count_by_host(mailbox, host);
}

msg_mailbox_t MSG_mailbox_get_by_alias(const char *alias)
{
  msg_mailbox_t mailbox = simcall_rdv_get_by_name(alias);

  if (!mailbox)
    mailbox = MSG_mailbox_new(alias);

  return mailbox;
}

/** \ingroup msg_mailbox_management
 * \brief Set the mailbox to receive in asynchronous mode
 *
 * All messages sent to this mailbox will be transferred to the receiver without waiting for the receive call.
 * The receive call will still be necessary to use the received data.
 * If there is a need to receive some messages asynchronously, and some not, two different mailboxes should be used.
 *
 * \param alias The name of the mailbox
 */
void MSG_mailbox_set_async(const char *alias){
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  simcall_rdv_set_receiver(mailbox, SIMIX_process_self());
  XBT_VERB("%s mailbox set to receive eagerly for process %p\n",alias, SIMIX_process_self());
}

/** \ingroup msg_mailbox_management
 * \brief Get a task from a mailbox on a given host
 *
 * \param mailbox The mailbox where the task was sent
 * \param task a memory location for storing a #msg_task_t.
 * \param host a #msg_host_t host from where the task was sent
 * \param timeout a timeout

 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, msg_task_t *task, msg_host_t host, double timeout)
{
  return MSG_mailbox_get_task_ext_bounded(mailbox, task, host, timeout, -1.0);
}

/** \ingroup msg_mailbox_management
 * \brief Get a task from a mailbox on a given host at a given rate
 *
 * \param mailbox The mailbox where the task was sent
 * \param task a memory location for storing a #msg_task_t.
 * \param host a #msg_host_t host from where the task was sent
 * \param timeout a timeout
 * \param rate a rate

 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_mailbox_get_task_ext_bounded(msg_mailbox_t mailbox, msg_task_t * task, msg_host_t host, double timeout,
                                             double rate)
{
  xbt_ex_t e;
  msg_error_t ret = MSG_OK;
  /* We no longer support getting a task from a specific host */
  if (host)
    THROW_UNIMPLEMENTED;

  TRACE_msg_task_get_start();
  double start_time = MSG_get_clock();

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_WARN("Asked to write the received task in a non empty struct -- proceeding.");

  /* Try to receive it by calling SIMIX network layer */
  TRY {
    simcall_comm_recv(MSG_process_self(), mailbox, task, NULL, NULL, NULL, NULL, timeout, rate);
    XBT_DEBUG("Got task %s from %p",(*task)->name,mailbox);
    if (msg_global->debug_multiple_use && (*task)->simdata->isused!=0)
      xbt_ex_free(*(xbt_ex_t*)(*task)->simdata->isused);
    (*task)->simdata->isused = 0;
  }
  CATCH(e) {
    switch (e.category) {
    case cancel_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    case host_error:
      ret = MSG_HOST_FAILURE;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }

  if (ret != MSG_HOST_FAILURE && ret != MSG_TRANSFER_FAILURE && ret != MSG_TIMEOUT) {
    TRACE_msg_task_get_end(start_time, *task);
  }
  MSG_RETURN(ret);
}

msg_error_t MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, msg_task_t task, double timeout)
{
  msg_error_t ret = MSG_OK;
  simdata_task_t t_simdata = NULL;
  msg_process_t process = MSG_process_self();
  simdata_process_t p_simdata = (simdata_process_t) SIMIX_process_self_get_data();

  int call_end = TRACE_msg_task_put_start(task);    //must be after CHECK_HOST()

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = ((simdata_process_t) SIMIX_process_self_get_data())->m_host;

  if (t_simdata->isused != 0) {
    if (msg_global->debug_multiple_use){
      XBT_ERROR("This task is already used in there:");
      xbt_backtrace_display((xbt_ex_t*) t_simdata->isused);
      XBT_ERROR("And you try to reuse it from here:");
      xbt_backtrace_display_current();
    } else {
      xbt_assert(t_simdata->isused == 0,
                 "This task is still being used somewhere else. You cannot send it now. Go fix your code!"
                 " (use --cfg=msg/debug_multiple_use:on to get the backtrace of the other process)");
    }
  }

  if (msg_global->debug_multiple_use)
    MSG_BT(t_simdata->isused, "Using Backtrace");
  else
    t_simdata->isused = (void*)1;
  t_simdata->comm = NULL;
  msg_global->sent_msg++;

  p_simdata->waiting_task = task;

  xbt_ex_t e;
  /* Try to send it by calling SIMIX network layer */
  TRY {
    smx_synchro_t comm = NULL; /* MC needs the comm to be set to NULL during the simix call  */
    comm = simcall_comm_isend(SIMIX_process_self(), mailbox,t_simdata->bytes_amount,
                              t_simdata->rate, task, sizeof(void *), NULL, NULL, NULL, task, 0);
    if (TRACE_is_enabled())
      simcall_set_category(comm, task->category);
     t_simdata->comm = comm;
     simcall_comm_wait(comm, timeout);
  }

  CATCH(e) {
    switch (e.category) {
    case cancel_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);

    /* If the send failed, it is not used anymore */
    if (msg_global->debug_multiple_use && t_simdata->isused!=0)
      xbt_ex_free(*(xbt_ex_t*)t_simdata->isused);
    t_simdata->isused = 0;
  }

  p_simdata->waiting_task = NULL;
  if (call_end)
    TRACE_msg_task_put_end();
  MSG_RETURN(ret);
}
