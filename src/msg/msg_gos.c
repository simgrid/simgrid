/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "msg_mailbox.h"
#include "mc/mc.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg,
                                "Logging specific to MSG (gos)");

/** \ingroup msg_task_usage
 * \brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of a process. It
 * takes only one parameter.
 * \param task a #msg_task_t to execute on the location on which the process is running.
 * \return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED
 * or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_task_execute(msg_task_t task)
{
  /* TODO: add this to other locations */
  msg_host_t host = MSG_process_get_host(MSG_process_self());
  MSG_host_add_task(host, task);

  msg_error_t ret = MSG_parallel_task_execute(task);

  MSG_host_del_task(host, task);

  return ret;
}

/** \ingroup msg_task_usage
 * \brief Executes a parallel task and waits for its termination.
 *
 * \param task a #msg_task_t to execute on the location on which the process is running.
 *
 * \return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED
 * or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_parallel_task_execute(msg_task_t task)
{
  xbt_ex_t e;
  simdata_task_t simdata = task->simdata;
  msg_process_t self = SIMIX_process_self();
  simdata_process_t p_simdata = SIMIX_process_self_get_data(self);
  e_smx_state_t comp_state;
  msg_error_t status = MSG_OK;

#ifdef HAVE_TRACING
  TRACE_msg_task_execute_start(task);
#endif

  xbt_assert((!simdata->compute) && (task->simdata->isused == 0),
             "This task is executed somewhere else. Go fix your code! %d",
             task->simdata->isused);

  XBT_DEBUG("Computing on %s", MSG_process_get_name(MSG_process_self()));

  if (simdata->computation_amount == 0 && !simdata->host_nb) {
#ifdef HAVE_TRACING
    TRACE_msg_task_execute_end(task);
#endif
    return MSG_OK;
  }


  TRY {

    simdata->isused=1;

    if (simdata->host_nb > 0) {
      simdata->compute = simcall_host_parallel_execute(task->name,
                                                       simdata->host_nb,
                                                       simdata->host_list,
                                                       simdata->comp_amount,
                                                       simdata->comm_amount,
                                                       1.0, -1.0);
      XBT_DEBUG("Parallel execution action created: %p", simdata->compute);
    } else {
      simdata->compute = simcall_host_execute(task->name,
                                              p_simdata->m_host,
                                              simdata->computation_amount,
                                              simdata->priority,
                                              simdata->bound,
                                              simdata->affinity_mask
                                              );

    }
#ifdef HAVE_TRACING
    simcall_set_category(simdata->compute, task->category);
#endif
    p_simdata->waiting_action = simdata->compute;
    comp_state = simcall_host_execution_wait(simdata->compute);

    p_simdata->waiting_action = NULL;

    simdata->isused=0;

    XBT_DEBUG("Execution task '%s' finished in state %d",
              task->name, (int)comp_state);
  }
  CATCH(e) {
    switch (e.category) {
    case cancel_error:
      status = MSG_TASK_CANCELED;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }
  /* action ended, set comm and compute = NULL, the actions is already destroyed
   * in the main function */
  simdata->computation_amount = 0.0;
  simdata->comm = NULL;
  simdata->compute = NULL;
#ifdef HAVE_TRACING
  TRACE_msg_task_execute_end(task);
#endif

  MSG_RETURN(status);
}


/** \ingroup msg_task_usage
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
msg_error_t MSG_process_sleep(double nb_sec)
{
  xbt_ex_t e;
  msg_error_t status = MSG_OK;
  /*msg_process_t proc = MSG_process_self();*/

#ifdef HAVE_TRACING
  TRACE_msg_process_sleep_in(MSG_process_self());
#endif

  /* create action to sleep */

  /*proc->simdata->waiting_action = act_sleep;

  FIXME: check if not setting the waiting_action breaks something on msg
  
  proc->simdata->waiting_action = NULL;*/

  TRY {
    simcall_process_sleep(nb_sec);
  }
  CATCH(e) {
    switch (e.category) {
    case cancel_error:
      status = MSG_TASK_CANCELED;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }

  #ifdef HAVE_TRACING
    TRACE_msg_process_sleep_out(MSG_process_self());
  #endif
  MSG_RETURN(status);
}

/** \ingroup msg_task_usage
 * \brief Deprecated function that used to receive a task from a mailbox from a specific host.
 *
 * Sorry, this function is not supported anymore. That wouldn't be
 * impossible to reimplement it, but we are lacking the time to do so ourselves.
 * If you need this functionality, you can either:
 *
 *  - implement the buffering mechanism on the user-level by queuing all messages
 *    received in the mailbox that do not match your expectation
 *  - change your application logic to leverage the mailboxes features. For example,
 *    if you have A receiving messages from B and C, you could have A waiting on
 *    mailbox "A" most of the time, but on "A#B" when it's waiting for specific
 *    messages from B and "A#C" when waiting for messages from C. You could even get A
 *    sometime waiting on all these mailboxes using @ref MSG_comm_waitany. You can find
 *    an example of use of this function in the @ref MSG_examples section.
 *  - Provide a proper patch to implement this functionality back in MSG. That wouldn't be
 *    very difficult actually. Check the function @ref MSG_mailbox_get_task_ext. During its call to
 *    simcall_comm_recv(), the 5th argument, match_fun, is NULL. Create a function that filters
 *    messages according to the host (that you will pass as sixth argument to simcall_comm_recv()
 *    and that your filtering function will receive as first parameter, and then, the filter could
 *    simply compare the host names, for example. After sufficient testing, provide an example that
 *    we could add to the distribution, and your first contribution to SimGrid is ready. Thanks in advance.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param host a #msg_host_t host from where the task was sent
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t
MSG_task_receive_from_host(msg_task_t * task, const char *alias,
                           msg_host_t host)
{
  return MSG_task_receive_ext(task, alias, -1, host);
}

/** msg_task_usage
 *\brief Deprecated function that used to receive a task from a mailbox from a specific host
 *\brief at a given rate
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param host a #msg_host_t host from where the task was sent
 * \param rate limit the reception to rate bandwidth
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t
MSG_task_receive_from_host_bounded(msg_task_t * task, const char *alias,
                           msg_host_t host, double rate)
{
  return MSG_task_receive_ext_bounded(task, alias, -1, host, rate);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox.
 *
 * This is a blocking function, the execution flow will be blocked
 * until the task is received. See #MSG_task_irecv
 * for receiving tasks asynchronously.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive(msg_task_t * task, const char *alias)
{
  return MSG_task_receive_with_timeout(task, alias, -1);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 *  \param rate limit the reception to rate bandwidth
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive_bounded(msg_task_t * task, const char *alias, double rate)
{
  return MSG_task_receive_with_timeout_bounded(task, alias, -1, rate);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked
 * until the task is received or the timeout is achieved. See #MSG_task_irecv
 * for receiving tasks asynchronously.  You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_receive_with_timeout(msg_task_t * task, const char *alias,
                              double timeout)
{
  return MSG_task_receive_ext(task, alias, timeout, NULL);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox with a given timeout and at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 *  \param rate limit the reception to rate bandwidth
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_receive_with_timeout_bounded(msg_task_t * task, const char *alias,
                              double timeout,double rate)
{
  return MSG_task_receive_ext_bounded(task, alias, timeout, NULL,rate);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox from a specific host with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked
 * until the task is received or the timeout is achieved. See #MSG_task_irecv
 * for receiving tasks asynchronously. You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * \param host a #msg_host_t host from where the task was sent
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
* #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_receive_ext(msg_task_t * task, const char *alias, double timeout,
                     msg_host_t host)
{
  xbt_ex_t e;
  msg_error_t ret = MSG_OK;
  XBT_DEBUG
      ("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'",
       alias);
  TRY {
    ret = MSG_mailbox_get_task_ext(MSG_mailbox_get_by_alias(alias), task,
                                   host, timeout);
  }
  CATCH(e) {
    switch (e.category) {
    case cancel_error:          /* may be thrown by MSG_mailbox_get_by_alias */
      ret = MSG_HOST_FAILURE;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }
  return ret;
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox from a specific host with a given timeout
 *  and at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * \param host a #msg_host_t host from where the task was sent
 * \param rate limit the reception to rate bandwidth
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
* #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_receive_ext_bounded(msg_task_t * task, const char *alias, double timeout,
                     msg_host_t host, double rate)
{
  XBT_DEBUG
      ("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'",
       alias);
  return MSG_mailbox_get_task_ext_bounded(MSG_mailbox_get_by_alias(alias), task,
                                  host, timeout, rate);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend(msg_task_t task, const char *alias)
{
  return MSG_task_isend_with_matching(task,alias,NULL,NULL);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox with a maximum rate
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication. The maxrate parameter allows the application
 * to limit the bandwidth utilization of network links when sending the task.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param maxrate the maximum communication rate for sending this task .
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend_bounded(msg_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_isend_with_matching(task,alias,NULL,NULL);
}


/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox, with support for matching requests
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param match_fun boolean function which parameters are:
 *        - match_data_provided_here
 *        - match_data_provided_by_other_side_if_any
 *        - the_smx_action_describing_the_other_side
 * \param match_data user provided data passed to match_fun
 * \return the msg_comm_t communication created
 */
XBT_INLINE msg_comm_t MSG_task_isend_with_matching(msg_task_t task, const char *alias,
    int (*match_fun)(void*,void*, smx_action_t),
    void *match_data)
{
  simdata_task_t t_simdata = NULL;
  msg_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

#ifdef HAVE_TRACING
  int call_end = TRACE_msg_task_put_start(task);
#endif

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = ((simdata_process_t) SIMIX_process_self_get_data(process))->m_host;

  xbt_assert(t_simdata->isused == 0,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->isused = 1;
  t_simdata->comm = NULL;
  msg_global->sent_msg++;

  /* Send it by calling SIMIX network layer */
  msg_comm_t comm = xbt_new0(s_msg_comm_t, 1);
  comm->task_sent = task;
  comm->task_received = NULL;
  comm->status = MSG_OK;
  comm->s_comm =
    simcall_comm_isend(mailbox, t_simdata->message_size,
                         t_simdata->rate, task, sizeof(void *), match_fun, NULL, match_data, 0);
  t_simdata->comm = comm->s_comm; /* FIXME: is the field t_simdata->comm still useful? */
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      simcall_set_category(comm->s_comm, task->category);
    }
#endif

#ifdef HAVE_TRACING
  if (call_end)
    TRACE_msg_task_put_end();
#endif

  return comm;
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter
 * is only called if the communication fails. If the communication does work,
 * it is responsibility of the receiver code to free anything related to
 * the task, as usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param cleanup a function to destroy the task if the
 * communication fails, e.g. MSG_task_destroy
 * (if NULL, no function will be called)
 */
void MSG_task_dsend(msg_task_t task, const char *alias, void_f_pvoid_t cleanup)
{
  simdata_task_t t_simdata = NULL;
  msg_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = ((simdata_process_t) SIMIX_process_self_get_data(process))->m_host;

  xbt_assert(t_simdata->isused == 0,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->isused = 1;
  t_simdata->comm = NULL;
  msg_global->sent_msg++;

#ifdef HAVE_TRACING
  int call_end = TRACE_msg_task_put_start(task);
#endif

  /* Send it by calling SIMIX network layer */
  smx_action_t comm = simcall_comm_isend(mailbox, t_simdata->message_size,
                       t_simdata->rate, task, sizeof(void *), NULL, cleanup, NULL, 1);
  t_simdata->comm = comm;
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      simcall_set_category(comm, task->category);
    }
#endif

#ifdef HAVE_TRACING
  if (call_end)
    TRACE_msg_task_put_end();
#endif
}


/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox with a maximal rate.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter
 * is only called if the communication fails. If the communication does work,
 * it is responsibility of the receiver code to free anything related to
 * the task, as usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param cleanup a function to destroy the task if the
 * communication fails, e.g. MSG_task_destroy
 * (if NULL, no function will be called)
 * \param maxrate the maximum communication rate for sending this task
 * 
 */
void MSG_task_dsend_bounded(msg_task_t task, const char *alias, void_f_pvoid_t cleanup, double maxrate)
{
  task->simdata->rate = maxrate;
  
  simdata_task_t t_simdata = NULL;
  msg_process_t process = MSG_process_self();
  msg_mailbox_t mailbox = MSG_mailbox_get_by_alias(alias);

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = ((simdata_process_t) SIMIX_process_self_get_data(process))->m_host;

  xbt_assert(t_simdata->isused == 0,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->isused = 1;
  t_simdata->comm = NULL;
  msg_global->sent_msg++;

#ifdef HAVE_TRACING
  int call_end = TRACE_msg_task_put_start(task);
#endif

  /* Send it by calling SIMIX network layer */
  smx_action_t comm = simcall_comm_isend(mailbox, t_simdata->message_size,
                       t_simdata->rate, task, sizeof(void *), NULL, cleanup, NULL, 1);
  t_simdata->comm = comm;
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      simcall_set_category(comm, task->category);
    }
#endif

#ifdef HAVE_TRACING
  if (call_end)
    TRACE_msg_task_put_end();
#endif
}

/** \ingroup msg_task_usage
 * \brief Starts listening for receiving a task from an asynchronous communication.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test()
 * to end the communication.
 * 
 * \param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * \param name of the mailbox to receive the task on
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv(msg_task_t *task, const char *name)
{
  smx_rdv_t rdv = MSG_mailbox_get_by_alias(name);

  /* FIXME: these functions are not traceable */

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_CRITICAL
        ("MSG_task_irecv() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  msg_comm_t comm = xbt_new0(s_msg_comm_t, 1);
  comm->task_sent = NULL;
  comm->task_received = task;
  comm->status = MSG_OK;
  comm->s_comm = simcall_comm_irecv(rdv, task, NULL, NULL, NULL);

  return comm;
}

/** \ingroup msg_task_usage
 * \brief Starts listening for receiving a task from an asynchronous communication
 * at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * \param name of the mailbox to receive the task on
 * \param rate limit the bandwidth to the given rate
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv_bounded(msg_task_t *task, const char *name, double rate)
{


  smx_rdv_t rdv = MSG_mailbox_get_by_alias(name);

  /* FIXME: these functions are not traceable */

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_CRITICAL
        ("MSG_task_irecv() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  msg_comm_t comm = xbt_new0(s_msg_comm_t, 1);
  comm->task_sent = NULL;
  comm->task_received = task;
  comm->status = MSG_OK;
  comm->s_comm = simcall_comm_irecv_bounded(rdv, task, NULL, NULL, NULL, rate);

  return comm;
}

/** \ingroup msg_task_usage
 * \brief Checks whether a communication is done, and if yes, finalizes it.
 * \param comm the communication to test
 * \return TRUE if the communication is finished
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 * or FALSE if the communication is not finished yet
 * If the status is FALSE, don't forget to use MSG_process_sleep() after the test.
 */
int MSG_comm_test(msg_comm_t comm)
{
  xbt_ex_t e;
  int finished = 0;

  TRY {
    finished = simcall_comm_test(comm->s_comm);

    if (finished && comm->task_received != NULL) {
      /* I am the receiver */
      (*comm->task_received)->simdata->isused = 0;
    }
  }
  CATCH(e) {
    switch (e.category) {
      case network_error:
        comm->status = MSG_TRANSFER_FAILURE;
        finished = 1;
        break;

      case timeout_error:
        comm->status = MSG_TIMEOUT;
        finished = 1;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }

  return finished;
}

/** \ingroup msg_task_usage
 * \brief This function checks if a communication is finished.
 * \param comms a vector of communications
 * \return the position of the finished communication if any
 * (but it may have failed, use MSG_comm_get_status() to know its status),
 * or -1 if none is finished
 */
int MSG_comm_testany(xbt_dynar_t comms)
{
  xbt_ex_t e;
  int finished_index = -1;

  /* create the equivalent dynar with SIMIX objects */
  xbt_dynar_t s_comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    xbt_dynar_push(s_comms, &comm->s_comm);
  }

  msg_error_t status = MSG_OK;
  TRY {
    finished_index = simcall_comm_testany(s_comms);
  }
  CATCH(e) {
    switch (e.category) {
      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;

      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }
  xbt_dynar_free(&s_comms);

  if (finished_index != -1) {
    comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
    /* the communication is finished */
    comm->status = status;

    if (status == MSG_OK && comm->task_received != NULL) {
      /* I am the receiver */
      (*comm->task_received)->simdata->isused = 0;
    }
  }

  return finished_index;
}

/** \ingroup msg_task_usage
 * \brief Destroys a communication.
 * \param comm the communication to destroy.
 */
void MSG_comm_destroy(msg_comm_t comm)
{
  xbt_free(comm);
}

/** \ingroup msg_task_usage
 * \brief Wait for the completion of a communication.
 *
 * It takes two parameters.
 * \param comm the communication to wait.
 * \param timeout Wait until the communication terminates or the timeout 
 * occurs. You can provide a -1 timeout to obtain an infinite timeout.
 * \return msg_error_t
 */
msg_error_t MSG_comm_wait(msg_comm_t comm, double timeout)
{
  xbt_ex_t e;
  TRY {
    simcall_comm_wait(comm->s_comm, timeout);

    if (comm->task_received != NULL) {
      /* I am the receiver */
      (*comm->task_received)->simdata->isused = 0;
    }

    /* FIXME: these functions are not traceable */
  }
  CATCH(e) {
    switch (e.category) {
    case network_error:
      comm->status = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      comm->status = MSG_TIMEOUT;
      break;
    default:
      RETHROW;
    }
    xbt_ex_free(e);
  }

  return comm->status;
}

/** \ingroup msg_task_usage
* \brief This function is called by a sender and permit to wait for each communication
*
* \param comm a vector of communication
* \param nb_elem is the size of the comm vector
* \param timeout for each call of MSG_comm_wait
*/
void MSG_comm_waitall(msg_comm_t * comm, int nb_elem, double timeout)
{
  int i = 0;
  for (i = 0; i < nb_elem; i++) {
    MSG_comm_wait(comm[i], timeout);
  }
}

/** \ingroup msg_task_usage
 * \brief This function waits for the first communication finished in a list.
 * \param comms a vector of communications
 * \return the position of the first finished communication
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 */
int MSG_comm_waitany(xbt_dynar_t comms)
{
  xbt_ex_t e;
  int finished_index = -1;

  /* create the equivalent dynar with SIMIX objects */
  xbt_dynar_t s_comms = xbt_dynar_new(sizeof(smx_action_t), NULL);
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    xbt_dynar_push(s_comms, &comm->s_comm);
  }

  msg_error_t status = MSG_OK;
  TRY {
    finished_index = simcall_comm_waitany(s_comms);
  }
  CATCH(e) {
    switch (e.category) {
      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;

      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;

      default:
        RETHROW;
    }
    xbt_ex_free(e);
  }

  xbt_assert(finished_index != -1, "WaitAny returned -1");
  xbt_dynar_free(&s_comms);

  comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
  /* the communication is finished */
  comm->status = status;

  if (comm->task_received != NULL) {
    /* I am the receiver */
    (*comm->task_received)->simdata->isused = 0;
  }

  return finished_index;
}

/**
 * \ingroup msg_task_usage
 * \brief Returns the error (if any) that occured during a finished communication.
 * \param comm a finished communication
 * \return the status of the communication, or #MSG_OK if no error occured
 * during the communication
 */
msg_error_t MSG_comm_get_status(msg_comm_t comm) {

  return comm->status;
}

/** \ingroup msg_task_usage
 * \brief Get a task (#msg_task_t) from a communication
 *
 * \param comm the communication where to get the task
 * \return the task from the communication
 */
msg_task_t MSG_comm_get_task(msg_comm_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  return comm->task_received ? *comm->task_received : comm->task_sent;
}

/**
 * \brief This function is called by SIMIX in kernel mode to copy the data of a comm.
 * \param comm the comm
 * \param buff the data copied
 * \param buff_size size of the buffer
 */
void MSG_comm_copy_data_from_SIMIX(smx_action_t comm, void* buff, size_t buff_size) {

  // copy the task
  SIMIX_comm_copy_pointer_callback(comm, buff, buff_size);

  // notify the user callback if any
  if (msg_global->task_copy_callback) {
    msg_task_t task = buff;
    msg_global->task_copy_callback(task,
        simcall_comm_get_src_proc(comm), simcall_comm_get_dst_proc(comm));
  }
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox
 *
 * This is a blocking function, the execution flow will be blocked
 * until the task is sent (and received in the other side if #MSG_task_receive is used).
 * See #MSG_task_isend for sending tasks asynchronously.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_send(msg_task_t task, const char *alias)
{
  XBT_DEBUG("MSG_task_send: Trying to send a message on mailbox '%s'", alias);
  return MSG_task_send_with_timeout(task, alias, -1);
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked
 * until the task is sent. The maxrate parameter allows the application
 * to limit the bandwidth utilization of network links when sending the task.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param maxrate the maximum communication rate for sending this task
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t
MSG_task_send_bounded(msg_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_send(task, alias);
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a timeout
 *
 * This is a blocking function, the execution flow will be blocked
 * until the task is sent or the timeout is achieved.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_send_with_timeout(msg_task_t task, const char *alias,
                           double timeout)
{
  return MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_alias(alias),
                                      task, timeout);
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a timeout and with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked
 * until the task is sent or the timeout is achieved.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 * \param maxrate the maximum communication rate for sending this task
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t
MSG_task_send_with_timeout_bounded(msg_task_t task, const char *alias,
                           double timeout, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_alias(alias),
                                      task, timeout);
}

/** \ingroup msg_task_usage
 * \brief Check if there is a communication going on in a mailbox.
 *
 * \param alias the name of the mailbox to be considered
 *
 * \return Returns 1 if there is a communication, 0 otherwise
 */
int MSG_task_listen(const char *alias)
{
  return !MSG_mailbox_is_empty(MSG_mailbox_get_by_alias(alias));
}

/** \ingroup msg_task_usage
 * \brief Check the number of communication actions of a given host pending in a mailbox.
 *
 * \param alias the name of the mailbox to be considered
 * \param host the host to check for communication
 *
 * \return Returns the number of pending communication actions of the host in the
 * given mailbox, 0 if there is no pending communication actions.
 *
 */
int MSG_task_listen_from_host(const char *alias, msg_host_t host)
{
  return
      MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_alias
                                               (alias), host);
}

/** \ingroup msg_task_usage
 * \brief Look if there is a communication on a mailbox and return the
 * PID of the sender process.
 *
 * \param alias the name of the mailbox to be considered
 *
 * \return Returns the PID of sender process,
 * -1 if there is no communication in the mailbox.
 */
int MSG_task_listen_from(const char *alias)
{
  msg_task_t task;

  if (NULL ==
      (task = MSG_mailbox_get_head(MSG_mailbox_get_by_alias(alias))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}

/** \ingroup msg_task_usage
 * \brief Sets the tracing category of a task.
 *
 * This function should be called after the creation of
 * a MSG task, to define the category of that task. The
 * first parameter task must contain a task that was
 * created with the function #MSG_task_create. The second
 * parameter category must contain a category that was
 * previously declared with the function #TRACE_category
 * (or with #TRACE_category_with_color).
 *
 * See \ref tracing for details on how to trace
 * the (categorized) resource utilization.
 *
 * \param task the task that is going to be categorized
 * \param category the name of the category to be associated to the task
 *
 * \see MSG_task_get_category, TRACE_category, TRACE_category_with_color
 */
void MSG_task_set_category (msg_task_t task, const char *category)
{
#ifdef HAVE_TRACING
  TRACE_msg_set_task_category (task, category);
#endif
}

/** \ingroup msg_task_usage
 *
 * \brief Gets the current tracing category of a task.
 *
 * \param task the task to be considered
 *
 * \see MSG_task_set_category
 *
 * \return Returns the name of the tracing category of the given task, NULL otherwise
 */
const char *MSG_task_get_category (msg_task_t task)
{
#ifdef HAVE_TRACING
  return task->category;
#else
  return NULL;
#endif
}

/**
 * \brief Returns the value of a given AS or router property
 *
 * \param asr the name of a router or AS
 * \param name a property name
 * \return value of a property (or NULL if property not set)
 */
const char *MSG_as_router_get_property_value(const char* asr, const char *name)
{
  return xbt_dict_get_or_null(MSG_as_router_get_properties(asr), name);
}

/**
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to
 * a the AS or router
 *
 * \param asr the name of a router or AS
 * \return a dict containing the properties
 */
xbt_dict_t MSG_as_router_get_properties(const char* asr)
{
  return (simcall_asr_get_properties(asr));
}

/**
 * \brief Change the value of a given AS or router
 *
 * \param asr the name of a router or AS
 * \param name a property name
 * \param value what to change the property to
 * \param free_ctn the freeing function to use to kill the value on need
 */
void MSG_as_router_set_property_value(const char* asr, const char *name, char *value,void_f_pvoid_t free_ctn) {
  xbt_dict_set(MSG_as_router_get_properties(asr), name, value,free_ctn);
}

#ifdef MSG_USE_DEPRECATED
/** \ingroup msg_deprecated_functions
 *
 * \brief Return the last value returned by a MSG function (except
 * MSG_get_errno...).
 */
msg_error_t MSG_get_errno(void)
{
  return PROCESS_GET_ERRNO();
}

/** \ingroup msg_deprecated_functions
 * \brief Put a task on a channel of an host and waits for the end of the
 * transmission.
 *
 * This function is used for describing the behavior of a process. It
 * takes three parameter.
 * \param task a #msg_task_t to send on another location. This task
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
 * \param channel the channel on which the process should put this
 task. This value has to be >=0 and < than the maximal number of
 channels fixed with MSG_set_channel_number().
 * \return #MSG_HOST_FAILURE if the host on which
 * this function was called was shut down,
 * #MSG_TRANSFER_FAILURE if the transfer could not be properly done
 * (network failure, dest failure) or #MSG_OK if it succeeded.
 */
msg_error_t MSG_task_put(msg_task_t task, msg_host_t dest, m_channel_t channel)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_send");
  return MSG_task_put_with_timeout(task, dest, channel, -1.0);
}

/** \ingroup msg_deprecated_functions
 * \brief Does exactly the same as MSG_task_put but with a bounded transmition
 * rate.
 *
 * \sa MSG_task_put
 */
msg_error_t
MSG_task_put_bounded(msg_task_t task, msg_host_t dest, m_channel_t channel,
                     double maxrate)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_send_bounded");
  task->simdata->rate = maxrate;
  return MSG_task_put(task, dest, channel);
}

/** \ingroup msg_deprecated_functions
 *
 * \brief Put a task on a channel of an
 * host (with a timeout on the waiting of the destination host) and
 * waits for the end of the transmission.
 *
 * This function is used for describing the behavior of a process. It
 * takes four parameter.
 * \param task a #msg_task_t to send on another location. This task
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
 * \param channel the channel on which the process should put this
 task. This value has to be >=0 and < than the maximal number of
 channels fixed with MSG_set_channel_number().
 * \param timeout the maximum time to wait for a task before giving
 up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task
 will not be modified
 * \return #MSG_HOST_FAILURE if the host on which
this function was called was shut down,
#MSG_TRANSFER_FAILURE if the transfer could not be properly done
(network failure, dest failure, timeout...) or #MSG_OK if the communication succeeded.
 */
msg_error_t
MSG_task_put_with_timeout(msg_task_t task, msg_host_t dest,
                          m_channel_t channel, double timeout)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_send_with_timeout");
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  XBT_DEBUG("MSG_task_put_with_timout: Trying to send a task to '%s'", SIMIX_host_get_name(dest->smx_host));
  return
      MSG_mailbox_put_with_timeout(MSG_mailbox_get_by_channel
                                   (dest, channel), task, timeout);
}

/** \ingroup msg_deprecated_functions
 * \brief Test whether there is a pending communication on a channel, and who sent it.
 *
 * It takes one parameter.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return -1 if there is no pending communication and the PID of the process who sent it otherwise
 */
int MSG_task_probe_from(m_channel_t channel)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_listen_from");
  msg_task_t task;

  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  if (NULL ==
      (task =
       MSG_mailbox_get_head(MSG_mailbox_get_by_channel
                            (MSG_host_self(), channel))))
    return -1;

  return MSG_process_get_PID(task->simdata->sender);
}

/** \ingroup msg_deprecated_functions
 * \brief Test whether there is a pending communication on a channel.
 *
 * It takes one parameter.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return 1 if there is a pending communication and 0 otherwise
 */
int MSG_task_Iprobe(m_channel_t channel)
{
  XBT_WARN("DEPRECATED!");
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
      !MSG_mailbox_is_empty(MSG_mailbox_get_by_channel
                            (MSG_host_self(), channel));
}

/** \ingroup msg_deprecated_functions

 * \brief Return the number of tasks waiting to be received on a \a
 channel and sent by \a host.
 *
 * It takes two parameters.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return the number of tasks waiting to be received on \a channel
 and sent by \a host.
 */
int MSG_task_probe_from_host(int channel, msg_host_t host)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_listen_from_host");
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
      MSG_mailbox_get_count_host_waiting_tasks(MSG_mailbox_get_by_channel
                                               (MSG_host_self(), channel),
                                               host);

}

/** \ingroup msg_deprecated_functions
 * \brief Listen on \a channel and waits for receiving a task from \a host.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #msg_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param host the host that is to be watched.
 * \return a #msg_error_t indicating whether the operation was successful (#MSG_OK), or why it failed otherwise.
 */
msg_error_t
MSG_task_get_from_host(msg_task_t * task, m_channel_t channel, msg_host_t host)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_receive_from_host");
  return MSG_task_get_ext(task, channel, -1, host);
}

/** \ingroup msg_deprecated_functions
 * \brief Listen on a channel and wait for receiving a task.
 *
 * It takes two parameters.
 * \param task a memory location for storing a #msg_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \return a #msg_error_t indicating whether the operation was successful (#MSG_OK), or why it failed otherwise.
 */
msg_error_t MSG_task_get(msg_task_t * task, m_channel_t channel)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_receive");
  return MSG_task_get_with_timeout(task, channel, -1);
}

/** \ingroup msg_deprecated_functions
 * \brief Listen on a channel and wait for receiving a task with a timeout.
 *
 * It takes three parameters.
 * \param task a memory location for storing a #msg_task_t. It will
 hold a task when this function will return. Thus \a task should not
 be equal to \c NULL and \a *task should be equal to \c NULL. If one of
 those two condition does not hold, there will be a warning message.
 * \param channel the channel on which the process should be
 listening. This value has to be >=0 and < than the maximal
 number of channels fixed with MSG_set_channel_number().
 * \param max_duration the maximum time to wait for a task before giving
 up. In such a case, #MSG_TRANSFER_FAILURE will be returned, \a task
 will not be modified and will still be
 equal to \c NULL when returning.
 * \return a #msg_error_t indicating whether the operation was successful (#MSG_OK), or why it failed otherwise.
 */
msg_error_t
MSG_task_get_with_timeout(msg_task_t * task, m_channel_t channel,
                          double max_duration)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_receive_with_timeout");
  return MSG_task_get_ext(task, channel, max_duration, NULL);
}

msg_error_t
MSG_task_get_ext(msg_task_t * task, m_channel_t channel, double timeout,
                 msg_host_t host)
{
  XBT_WARN("DEPRECATED! Now use MSG_task_receive_ext");
  xbt_assert((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return
      MSG_mailbox_get_task_ext(MSG_mailbox_get_by_channel
                               (MSG_host_self(), channel), task, host,
                               timeout);
}

#endif
