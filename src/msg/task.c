/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

/** \defgroup m_task_management Managing functions of Tasks
 *  \brief This section describes the task structure of MSG
 *  (#m_task_t) and the functions for managing it.
 */
/** @addtogroup m_task_management
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Tasks" --> \endhtmlonly
 * 
 *  Since most scheduling algorithms rely on a concept of task
 *  that can be either <em>computed</em> locally or
 *  <em>transferred</em> on another processor, it seems to be the
 *  right level of abstraction for our purposes. A <em>task</em>
 *  may then be defined by a <em>computing amount</em>, a
 *  <em>message size</em> and some <em>private data</em>.
 */

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
   MSG_task_get() and MSG_task_put(). This value has to be >=0.
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
  simdata->refcount = 1;
  simdata->sender = NULL;
  simdata->receiver = NULL;
  simdata->cond = SIMIX_cond_init();
  simdata->mutex = SIMIX_mutex_init();
  simdata->compute = NULL;
  simdata->comm = NULL;

  simdata->host_list = NULL;
  simdata->comp_amount = NULL;
  simdata->comm_amount = NULL;
#ifdef HAVE_TRACING
  TRACE_msg_task_create (task);
#endif

  return task;
}

/** \ingroup m_task_management
 * \brief Return the user data of a #m_task_t.
 *
 * This function checks whether \a task is a valid pointer or not and return
   the user data associated to \a task if it is possible.
 */
void *MSG_task_get_data(m_task_t task)
{
  xbt_assert0((task != NULL), "Invalid parameter");

  return (task->data);
}

/** \ingroup m_task_management
 * \brief Sets the user data of a #m_task_t.
 *
 * This function allows to associate a new pointer to
   the user data associated of \a task.
 */
void MSG_task_set_data(m_task_t task,void *data)
{
  xbt_assert0((task != NULL), "Invalid parameter");

  task->data = data;
}

/** \ingroup m_task_management
 * \brief Return the sender of a #m_task_t.
 *
 * This functions returns the #m_process_t which sent this task
 */
m_process_t MSG_task_get_sender(m_task_t task)
{
  xbt_assert0(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->sender;
}

/** \ingroup m_task_management
 * \brief Return the source of a #m_task_t.
 *
 * This functions returns the #m_host_t from which this task was sent
 */
m_host_t MSG_task_get_source(m_task_t task)
{
  xbt_assert0(task, "Invalid parameters");
  return ((simdata_task_t) task->simdata)->source;
}

/** \ingroup m_task_management
 * \brief Return the name of a #m_task_t.
 *
 * This functions returns the name of a #m_task_t as specified on creation
 */
const char *MSG_task_get_name(m_task_t task)
{
  xbt_assert0(task, "Invalid parameters");
  return task->name;
}

void MSG_task_refcount_dec(m_task_t task)
{
  task->simdata->refcount--;
}

/** \ingroup m_task_management
 * \brief Destroy a #m_task_t.
 *
 * Destructor for #m_task_t. Note that you should free user data, if any, \b 
   before calling this function.
 */
MSG_error_t MSG_task_destroy(m_task_t task)
{
  smx_action_t action = NULL;
  xbt_assert0((task != NULL), "Invalid parameter");

  /* why? if somebody is using, then you can't free! ok... but will return MSG_OK? when this task will be destroyed? isn't the user code wrong? */
  task->simdata->refcount--;
  if (task->simdata->refcount > 0)
    return MSG_OK;
#ifdef HAVE_TRACING
  TRACE_msg_task_destroy (task);
#endif

  if (task->name)
    free(task->name);

  SIMIX_cond_destroy(task->simdata->cond);
  SIMIX_mutex_destroy(task->simdata->mutex);

  action = task->simdata->compute;
  if (action)
    SIMIX_action_destroy(action);
  
  /* parallel tasks only */
  if (task->simdata->host_list)
    xbt_free(task->simdata->host_list);

  /* free main structures */
  xbt_free(task->simdata);
  xbt_free(task);

  return MSG_OK;
}


/** \ingroup m_task_management
 * \brief Cancel a #m_task_t.
 * \param task the taskt to cancel. If it was executed or transfered, it 
          stops the process that were working on it.
 */
MSG_error_t MSG_task_cancel(m_task_t task)
{
  xbt_assert0((task != NULL), "Invalid parameter");

  if (task->simdata->compute) {
    SIMIX_action_cancel(task->simdata->compute);
    return MSG_OK;
  }
  if (task->simdata->comm) {
    SIMIX_communication_cancel(task->simdata->comm);
    return MSG_OK;
  }
  THROW_IMPOSSIBLE;
}

/** \ingroup m_task_management
 * \brief Returns the computation amount needed to process a task #m_task_t.
 *        Once a task has been processed, this amount is thus set to 0...
 */
double MSG_task_get_compute_duration(m_task_t task)
{
  xbt_assert0((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->computation_amount;
}

/** \ingroup m_task_management
 * \brief Returns the remaining computation amount of a task #m_task_t.
 *
 */
double MSG_task_get_remaining_computation(m_task_t task)
{
  xbt_assert0((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  if (task->simdata->compute) {
    return SIMIX_action_get_remains(task->simdata->compute);
  } else {
    return task->simdata->computation_amount;
  }
}



/** \ingroup m_task_management
 * \brief Returns the total amount received by a task #m_task_t.
 *
 */
double MSG_task_get_remaining_communication(m_task_t task)
{
  xbt_assert0((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  return SIMIX_communication_get_remains(task->simdata->comm);
}

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #m_task_t.
 *
 */
double MSG_task_get_data_size(m_task_t task)
{
  xbt_assert0((task != NULL)
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
  xbt_assert0((task != NULL)
              && (task->simdata != NULL), "Invalid parameter");

  task->simdata->priority = 1 / priority;
  if (task->simdata->compute)
    SIMIX_action_set_priority(task->simdata->compute,
                              task->simdata->priority);
}


