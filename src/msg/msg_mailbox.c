/* Mailboxes in MSG */

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mailbox.h"
#include "msg/private.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_mailbox, msg,
                                "Logging specific to MSG (mailbox)");

msg_mailbox_t MSG_mailbox_new(const char *alias)
{
  return SIMIX_req_rdv_create(alias ? xbt_strdup(alias) : NULL);
}

void MSG_mailbox_free(void *mailbox)
{
  SIMIX_req_rdv_destroy((msg_mailbox_t)mailbox);
}

int MSG_mailbox_is_empty(msg_mailbox_t mailbox)
{
  return (NULL == SIMIX_req_rdv_get_head(mailbox));
}

m_task_t MSG_mailbox_get_head(msg_mailbox_t mailbox)
{
  smx_action_t comm = SIMIX_req_rdv_get_head(mailbox);

  if (!comm)
    return NULL;

  return (m_task_t) SIMIX_req_comm_get_data(comm);
}

int
MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox,
                                         m_host_t host)
{
  return SIMIX_req_rdv_comm_count_by_host(mailbox,
                                      host->simdata->smx_host);
}

msg_mailbox_t MSG_mailbox_get_by_alias(const char *alias)
{

  msg_mailbox_t mailbox = SIMIX_req_rdv_get_by_name(alias);

  if (!mailbox)
    mailbox = MSG_mailbox_new(alias);

  return mailbox;
}

msg_mailbox_t MSG_mailbox_get_by_channel(m_host_t host,
                                         m_channel_t channel)
{
  xbt_assert0((host != NULL), "Invalid host");
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return host->simdata->mailboxes[(size_t) channel];
}

MSG_error_t
MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, m_task_t * task,
                         m_host_t host, double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret = MSG_OK;
  smx_action_t comm = NULL;
#ifdef HAVE_TRACING
  double start_time = 0;
#endif
  /* We no longer support getting a task from a specific host */
  if (host)
    THROW_UNIMPLEMENTED;

  CHECK_HOST();
#ifdef HAVE_TRACING
  TRACE_msg_task_get_start();
  start_time = MSG_get_clock();
#endif

  /* Sanity check */
  xbt_assert0(task, "Null pointer for the task storage");

  if (*task)
    CRITICAL0
        ("MSG_task_get() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  TRY {
    comm = SIMIX_req_comm_irecv(mailbox, task, NULL);
    SIMIX_req_comm_wait(comm, timeout);
    SIMIX_req_comm_destroy(comm);
    DEBUG2("Got task %s from %p",(*task)->name,mailbox);
    (*task)->simdata->refcount--;
  }
  CATCH(e) {
    switch (e.category) {
    case host_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    default:
    	xbt_backtrace_display(&e);
      xbt_die(bprintf("Unhandled SIMIX network exception: %s", e.msg));
    }
    xbt_ex_free(e);
  }

  if (ret != MSG_HOST_FAILURE &&
      ret != MSG_TRANSFER_FAILURE && ret != MSG_TIMEOUT) {
#ifdef HAVE_TRACING
    TRACE_msg_task_get_end(start_time, *task);
#endif
  }
  MSG_RETURN(ret);
}

MSG_error_t
MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, m_task_t task,
                             double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret = MSG_OK;
  simdata_task_t t_simdata = NULL;
  m_process_t process = MSG_process_self();
#ifdef HAVE_TRACING
  int call_end = 0;
#endif
  CHECK_HOST();

#ifdef HAVE_TRACING
  call_end = TRACE_msg_task_put_start(task);    //must be after CHECK_HOST()
#endif


  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  xbt_assert0(t_simdata->refcount == 1,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->refcount++;
  msg_global->sent_msg++;

  process->simdata->waiting_task = task;

  /* Try to send it by calling SIMIX network layer */
  TRY {
    t_simdata->comm = SIMIX_req_comm_isend(mailbox, t_simdata->message_size,
                       t_simdata->rate, task, sizeof(void *), task);
#ifdef HAVE_TRACING
    SIMIX_req_set_category(t_simdata->comm, task->category);
#endif
    SIMIX_req_comm_wait(t_simdata->comm, timeout);
    SIMIX_req_comm_destroy(t_simdata->comm);
  }

  CATCH(e) {
    switch (e.category) {
    case host_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    default:
      xbt_die(bprintf("Unhandled SIMIX network exception: %s", e.msg));
    }
    xbt_ex_free(e);

    /* Decrement the refcount only on failure */
    t_simdata->refcount--;
  }

  process->simdata->waiting_task = NULL;
#ifdef HAVE_TRACING
  if (call_end)
    TRACE_msg_task_put_end();
#endif
  MSG_RETURN(ret);
}
