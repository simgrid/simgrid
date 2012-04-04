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
 * \brief Creates a new #m_task_t.
 *
 * A constructor for #m_task_t taking four arguments and returning the 
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
 * \see m_task_t
 * \return The new corresponding object.
 */
m_task_t MSG_task_create(const char *name, double compute_duration,
                         double message_size, void *data)
{
  m_task_t task = xbt_new(s_m_task_t, 1);
  simdata_task_t simdata = xbt_new(s_simdata_task_t, 1);
  task->simdata = simdata;
  /* Task structure */
  task->name = xbt_strdup(name);
  task->data = data;

  /* Simulator Data */
  simdata->host_nb = 0;
  simdata->computation_amount = compute_duration;
  simdata->message_size = message_size;
  simdata->rate = -1.0;
  simdata->priority = 1.0;
  simdata->isused = 0;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->compute = NULL;
  simdata->comm = NULL;

  simdata->host_list = NULL;
  simdata->comp_amount = NULL;
  simdata->comm_amount = NULL;
#ifdef HAVE_TRACING
  TRACE_msg_task_create(task);
#endif

  return task;
}

/*************** Begin GPU ***************/
/** \ingroup m_task_management
 * \brief Creates a new #m_gpu_task_t.

 * A constructor for #m_gpu_task_t taking four arguments and returning
   a pointer to the new created GPU task.

 * \param name a name for the object. It is for user-level information
   and can be NULL.

 * \param compute_duration a value of the processing amount (in flop)
   needed to process this new task. If 0, then it cannot be executed with
   MSG_gpu_task_execute(). This value has to be >=0.

 * \param dispatch_latency time in seconds to load this task on the GPU

 * \param collect_latency time in seconds to transfer result from the GPU
   back to the CPU (host) when done

 * \see m_gpu_task_t
 * \return The new corresponding object.
 */
m_gpu_task_t MSG_gpu_task_create(const char *name, double compute_duration,
                         double dispatch_latency, double collect_latency)
{
  m_gpu_task_t task = xbt_new(s_m_gpu_task_t, 1);
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
 * \brief Return the user data of a #m_task_t.
 *
 * This function checks whether \a task is a valid pointer or not and return
   the user data associated to \a task if it is possible.
 */
void *MSG_task_get_data(m_task_t task)
{
  xbt_assert((task != NULL), "Invalid parameter");

  return (task->data);
}

/** \ingroup m_task_management
 * \brief Sets the user data of a #m_task_t.
 *
 * This function allows to associate a new pointer to
   the user data associated of \a task.
 */
void MSG_task_set_data(m_task_t task, void *data)
{
  xbt_assert((task != NULL), "Invalid parameter");

  task->data = data;
}

/** \ingroup m_task_management
 * \brief Sets a function to be called when a task has just been copied.
 * \param callback a callback function
 */
void MSG_task_set_copy_callback(void (*callback)
    (m_task_t task, m_process_t sender, m_process_t receiver)) {

  msg_global->task_copy_callback = callback;

  if (callback) {
    SIMIX_comm_set_copy_data_callback(MSG_comm_copy_data_from_SIMIX);
  }
  else {
    SIMIX_comm_set_copy_data_callback(SIMIX_comm_copy_pointer_callback);
  }
}

/** \ingroup m_task_management
 * \brief Return the sender of a #m_task_t.
 *
 * This functions returns the #m_process_t which sent this task
 */
m_process_t MSG_task_get_sender(m_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->sender;
}

/** \ingroup m_task_management
 * \brief Return the source of a #m_task_t.
 *
 * This functions returns the #m_host_t from which this task was sent
 */
m_host_t MSG_task_get_source(m_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->source;
}

/** \ingroup m_task_management
 * \brief Return the name of a #m_task_t.
 *
 * This functions returns the name of a #m_task_t as specified on creation
 */
const char *MSG_task_get_name(m_task_t task)
{
  xbt_assert(task, "Invalid parameters");
  return task->name;
}

/** \ingroup m_task_management
 * \brief Return the name of a #m_task_t.
 *
 * This functions allows to associate a name to a task
 */
void MSG_task_set_name(m_task_t task, const char *name)
{
  xbt_assert(task, "Invalid parameters");
  task->name = xbt_strdup(name);
}

/** \ingroup m_task_management
 * \brief Destroy a #m_task_t.
 *
 * Destructor for #m_task_t. Note that you should free user data, if any, \b 
 * before calling this function.
 *
 * Only the process that owns the task can destroy it.
 * The owner changes after a successful send.
 * If a task is successfully sent, the receiver becomes the owner and is
 * supposed to destroy it. The sender should not use it anymore.
 * If the task failed to be sent, the sender remains the owner of the task.
 */
MSG_error_t MSG_task_destroy(m_task_t task)
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


/** \ingroup m_task_management
 * \brief Cancel a #m_task_t.
 * \param task the task to cancel. If it was executed or transfered, it
          stops the process that were working on it.
 */
MSG_error_t MSG_task_cancel(m_task_t task)
{
  xbt_assert((task != NULL), "Invalid parameter");

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
 * \brief Returns the computation amount needed to process a task #m_task_t.
 *        Once a task has been processed, this amount is thus set to 0...
 */
double MSG_task_get_compute_duration(m_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->computation_amount;
}


/** \ingroup m_task_management
 * \brief set the computation amount needed to process a task #m_task_t.
 */

void MSG_task_set_compute_duration(m_task_t task,
                                   double computation_amount)
{
  xbt_assert(task, "Invalid parameter");
  task->simdata->computation_amount = computation_amount;

}

/** \ingroup m_task_management
 * \brief Returns the remaining computation amount of a task #m_task_t.
 *
 */
double MSG_task_get_remaining_computation(m_task_t task)
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
 * \brief Returns the total amount received by a task #m_task_t.
 *        If the communication does not exist it will return 0.
 *        So, if the communication has FINISHED or FAILED it returns
 *        zero.
 */
double MSG_task_get_remaining_communication(m_task_t task)
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
int MSG_task_is_latency_bounded(m_task_t task)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");
  XBT_DEBUG("calling simcall_communication_is_latency_bounded(%p)",
         task->simdata->comm);
  return simcall_comm_is_latency_bounded(task->simdata->comm);
}
#endif

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #m_task_t.
 *
 */
double MSG_task_get_data_size(m_task_t task)
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
void MSG_task_set_priority(m_task_t task, double priority)
{
  xbt_assert((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  task->simdata->priority = 1 / priority;
  if (task->simdata->compute)
    simcall_host_execution_set_priority(task->simdata->compute,
                                      task->simdata->priority);
}
