/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** @addtogroup m_task_management
 *    
 * 
 *  Since most scheduling algorithms rely on a concept of task
 *  that can be either <em>computed</em> locally or
 *  <em>transferred</em> on another processor, it seems to be the
 *  right level of abstraction for our purposes. A <em>task</em>
 *  may then be defined by a <em>computing amount</em>, a
 *  <em>message size</em> and some <em>private data</em>.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_task, msg,
                                "Logging specific to MSG (task)");

/********************************* Task **************************************/
/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t.
 *
 * A constructor for #msg_task_t taking four arguments and returning the
   corresponding object.
 * \param name a name for the object. It is for user-level information
   and can be NULL.
 * \param compute_duration a value of the processing amount (in flop)
   needed to process this new task. If 0, then it cannot be executed with
   MSG_task_execute(). This value has to be >=0.
 * \param message_size a value of the amount of data (in bytes) needed to
   transfer this new task. If 0, then it cannot be transfered with
   MSG_task_send() and MSG_task_recv(). This value has to be >=0.
 * \param data a pointer to any data may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t MSG_task_create(const char *name, double compute_duration,
                         double message_size, void *data)
{
  msg_task_t task = xbt_new(s_msg_task_t, 1);
  simdata_task_t simdata = xbt_new(s_simdata_task_t, 1);
  task->simdata = simdata;

  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->compute = NULL;
  simdata->comm = NULL;
  simdata->message_size = message_size;
  simdata->computation_amount = compute_duration;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->source = NULL;
  simdata->priority = 1.0;
  simdata->bound = 0;
  simdata->affinity_mask = 0;
  simdata->rate = -1.0;
  simdata->isused = 0;

  simdata->host_nb = 0;
  simdata->host_list = NULL;
  simdata->comp_amount = NULL;
  simdata->comm_amount = NULL;
#ifdef HAVE_TRACING
  TRACE_msg_task_create(task);
#endif

  return task;
}

/** \ingroup m_task_management
 * \brief Creates a new #msg_task_t (a parallel one....).
 *
 * A constructor for #msg_task_t taking six arguments and returning the
 corresponding object.
 * \param name a name for the object. It is for user-level information
 and can be NULL.
 * \param host_nb the number of hosts implied in the parallel task.
 * \param host_list an array of \p host_nb msg_host_t.
 * \param computation_amount an array of \p host_nb
 doubles. computation_amount[i] is the total number of operations
 that have to be performed on host_list[i].
 * \param communication_amount an array of \p host_nb* \p host_nb doubles.
 * \param data a pointer to any data may want to attach to the new
 object.  It is for user-level information and can be NULL. It can
 be retrieved with the function \ref MSG_task_get_data.
 * \see msg_task_t
 * \return The new corresponding object.
 */
msg_task_t
MSG_parallel_task_create(const char *name, int host_nb,
                         const msg_host_t * host_list,
                         double *computation_amount,
                         double *communication_amount, void *data)
{
  msg_task_t task = MSG_task_create(name, 0, 0, data);
  simdata_task_t simdata = task->simdata;
  int i;

  /* Simulator Data specific to parallel tasks */
  simdata->host_nb = host_nb;
  simdata->host_list = xbt_new0(smx_host_t, host_nb);
  simdata->comp_amount = computation_amount;
  simdata->comm_amount = communication_amount;

  for (i = 0; i < host_nb; i++)
    simdata->host_list[i] = host_list[i];

  return task;
}

/*************** Begin GPU ***************/
/** \ingroup m_task_management
 * \brief Creates a new #msg_gpu_task_t.

 * A constructor for #msg_gpu_task_t taking four arguments and returning
   a pointer to the new created GPU task.

 * \param name a name for the object. It is for user-level information
   and can be NULL.

 * \param compute_duration a value of the processing amount (in flop)
   needed to process this new task. If 0, then it cannot be executed with
   MSG_gpu_task_execute(). This value has to be >=0.

 * \param dispatch_latency time in seconds to load this task on the GPU

 * \param collect_latency time in seconds to transfer result from the GPU
   back to the CPU (host) when done

 * \see msg_gpu_task_t
 * \return The new corresponding object.
 */
msg_gpu_task_t MSG_gpu_task_create(const char *name, double compute_duration,
                         double dispatch_latency, double collect_latency)
{
  msg_gpu_task_t task = xbt_new(s_msg_gpu_task_t, 1);
  simdata_gpu_task_t simdata = xbt_new(s_simdata_gpu_task_t, 1);
  task->simdata = simdata;
  /* Task structure */
  task->name = xbt_strdup(name);

  /* Simulator Data */
  simdata->computation_amount = compute_duration;
  simdata->dispatch_latency   = dispatch_latency;
  simdata->collect_latency    = collect_latency;

#ifdef HAVE_TRACING
  //FIXME
  /* TRACE_msg_gpu_task_create(task); */
#endif

  return task;
}
/*************** End GPU ***************/

/** \ingroup m_task_management
 * \brief Return the user data of a #msg_task_t.
 *
 * This function checks whether \a task is a valid pointer or not and return
   the user data associated to \a task if it is possible.
 */
void *MSG_task_get_data(msg_task_t task)
{
  xbt_assert((task != NULL), "Invalid parameter");

  return (task->data);
}

/** \ingroup m_task_management
 * \brief Sets the user data of a #msg_task_t.
 *
 * This function allows to associate a new pointer to
   the user data associated of \a task.
 */
void MSG_task_set_data(msg_task_t task, void *data)
{
  xbt_assert((task != NULL), "Invalid parameter");

  task->data = data;
}

/** \ingroup m_task_management
 * \brief Sets a function to be called when a task has just been copied.
 * \param callback a callback function
 */
void MSG_task_set_copy_callback(void (*callback)
    (msg_task_t task, msg_process_t sender, msg_process_t receiver)) {

  msg_global->task_copy_callback = callback;

  if (callback) {
    SIMIX_comm_set_copy_data_callback(MSG_comm_copy_data_from_SIMIX);
  }
  else {
    SIMIX_comm_set_copy_data_callback(SIMIX_comm_copy_pointer_callback);
  }
}

/** \ingroup m_task_management
 * \brief Return the sender of a #msg_task_t.
 *
 * This functions returns the #msg_process_t which sent this task
 */
msg_process_t MSG_task_get_sender(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->sender;
}

/** \ingroup m_task_management
 * \brief Return the source of a #msg_task_t.
 *
 * This functions returns the #msg_host_t from which this task was sent
 */
msg_host_t MSG_task_get_source(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->source;
}

/** \ingroup m_task_management
 * \brief Return the name of a #msg_task_t.
 *
 * This functions returns the name of a #msg_task_t as specified on creation
 */
const char *MSG_task_get_name(msg_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return task->name;
}

/** \ingroup m_task_management
 * \brief Sets the name of a #msg_task_t.
 *
 * This functions allows to associate a name to a task
 */
void MSG_task_set_name(msg_task_t task, const char *name)
{
  xbt_assert(task, "Invalid parameters");
  task->name = xbt_strdup(name);
}

/** \ingroup m_task_management
 * \brief Destroy a #msg_task_t.
 *
 * Destructor for #msg_task_t. Note that you should free user data, if any, \b
 * before calling this function.
 *
 * Only the process that owns the task can destroy it.
 * The owner changes after a successful send.
 * If a task is successfully sent, the receiver becomes the owner and is
 * supposed to destroy it. The sender should not use it anymore.
 * If the task failed to be sent, the sender remains the owner of the task.
 */
msg_error_t MSG_task_destroy(msg_task_t task)
{
  smx_action_t action = NULL;
  xbt_assert((task != NULL), "Invalid parameter");

  if (task->simdata->isused) {
    /* the task is being sent or executed: cancel it first */
    MSG_task_cancel(task);
  }
#ifdef HAVE_TRACING
  TRACE_msg_task_destroy(task);
#endif

  xbt_free(task->name);

  action = task->simdata->compute;
  if (action)
    simcall_host_execution_destroy(action);

  /* parallel tasks only */
  xbt_free(task->simdata->host_list);

  /* free main structures */
  xbt_free(task->simdata);
  xbt_free(task);

  return MSG_OK;
}


/** \ingroup m_task_usage
 * \brief Cancel a #msg_task_t.
 * \param task the task to cancel. If it was executed or transfered, it
          stops the process that were working on it.
 */
msg_error_t MSG_task_cancel(msg_task_t task)
{
  xbt_assert((task != NULL), "Cannot cancel a NULL task");

  if (task->simdata->compute) {
    simcall_host_execution_cancel(task->simdata->compute);
  }
  else if (task->simdata->comm) {
    simcall_comm_cancel(task->simdata->comm);
    task->simdata->isused = 0;
  }
  return MSG_OK;
}

/** \ingroup m_task_management
 * \brief Returns the computation amount needed to process a task #msg_task_t.
 *
 * Once a task has been processed, this amount is set to 0. If you want, you
 * can reset this value with #MSG_task_set_compute_duration before restarting the task.
 */
double MSG_task_get_compute_duration(msg_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->computation_amount;
}


/** \ingroup m_task_management
 * \brief set the computation amount needed to process a task #msg_task_t.
 *
 * \warning If the computation is ongoing (already started and not finished),
 * it is not modified by this call. And the termination of the ongoing task with
 * set the computation_amount to zero, overriding any value set during the
 * execution.
 */

void MSG_task_set_compute_duration(msg_task_t task,
                                   double computation_amount)
{
  xbt_assert(task, "Invalid parameter");
  task->simdata->computation_amount = computation_amount;

}

/** \ingroup m_task_management
 * \brief set the amount data attached with a task #msg_task_t.
 *
 * \warning If the transfer is ongoing (already started and not finished),
 * it is not modified by this call. 
 */

void MSG_task_set_data_size(msg_task_t task,
                                   double data_size)
{
  xbt_assert(task, "Invalid parameter");
  task->simdata->message_size = data_size;

}



/** \ingroup m_task_management
 * \brief Returns the remaining computation amount of a task #msg_task_t.
 *
 * If the task is ongoing, this call retrieves the remaining amount of work.
 * If it is not ongoing, it returns the total amount of work that will be
 * executed when the task starts.
 */
double MSG_task_get_remaining_computation(msg_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  if (task->simdata->compute) {
    return simcall_host_execution_get_remains(task->simdata->compute);
  } else {
    return task->simdata->computation_amount;
  }
}

/** \ingroup m_task_management
 * \brief Returns the total amount received by a task #msg_task_t.
 *        If the communication does not exist it will return 0.
 *        So, if the communication has FINISHED or FAILED it returns
 *        zero.
 */
double MSG_task_get_remaining_communication(msg_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");
  XBT_DEBUG("calling simcall_communication_get_remains(%p)",
         task->simdata->comm);
  return simcall_comm_get_remains(task->simdata->comm);
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
/** \ingroup m_task_management
 * \brief Return 1 if communication task is limited by latency, 0 otherwise
 *
 */
int MSG_task_is_latency_bounded(msg_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");
  XBT_DEBUG("calling simcall_communication_is_latency_bounded(%p)",
         task->simdata->comm);
  return simcall_comm_is_latency_bounded(task->simdata->comm);
}
#endif

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #msg_task_t.
 *
 */
double MSG_task_get_data_size(msg_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->message_size;
}



/** \ingroup m_task_management
 * \brief Changes the priority of a computation task. This priority doesn't affect 
 *        the transfer rate. A priority of 2 will make a task receive two times more
 *        cpu power than the other ones.
 *
 */
void MSG_task_set_priority(msg_task_t task, double priority)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  task->simdata->priority = 1 / priority;
  if (task->simdata->compute)
    simcall_host_execution_set_priority(task->simdata->compute,
                                      task->simdata->priority);
}


/** \ingroup m_task_management
 * \brief Changes the maximum CPU utilization of a computation task.
 *        Unit is flops/s.
 *
 * For VMs, there is a pitfall. Please see MSG_vm_set_bound().
 */
void MSG_task_set_bound(msg_task_t task, double bound)
{
  xbt_assert(task, "Invalid parameter");
  xbt_assert(task->simdata, "Invalid parameter");

  if (bound == 0)
    XBT_INFO("bound == 0 means no capping (i.e., unlimited).");

  task->simdata->bound = bound;
  if (task->simdata->compute)
    simcall_host_execution_set_bound(task->simdata->compute,
                                      task->simdata->bound);
}


/** \ingroup m_task_management
 * \brief Changes the CPU affinity of a computation task.
 *
 */
void MSG_task_set_affinity(msg_task_t task, msg_host_t host, unsigned long mask)
{
  xbt_assert(task, "Invalid parameter");
  xbt_assert(task->simdata, "Invalid parameter");

  task->simdata->affinity_mask = mask;
  if (task->simdata->compute)
    simcall_host_execution_set_affinity(task->simdata->compute, host, mask);
}
