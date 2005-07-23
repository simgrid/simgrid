/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(task, msg,
				"Logging specific to MSG (task)");

static char sprint_buffer[64];

/** \defgroup m_task_management Managing functions of Tasks
 *  \brief This section describes the task structure of MSG
 *  (#m_task_t) and the functions for managing it.
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
 * \param compute_duration a value of the processing amount (in Mflop)
   needed to process this new task. If 0, then it cannot be executed with
   MSG_task_execute(). This value has to be >=0.
 * \param message_size a value of the amount of data (in Mb) needed to
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
  simdata_task_t simdata = xbt_new0(s_simdata_task_t,1);
  m_task_t task = xbt_new0(s_m_task_t,1);
  
  /* Task structure */
  task->name = xbt_strdup(name);
  task->simdata = simdata;
  task->data = data;

  /* Simulator Data */
  simdata->sleeping = xbt_dynar_new(sizeof(m_process_t),NULL);
  simdata->computation_amount = compute_duration;
  simdata->message_size = message_size;
  simdata->rate = -1.0;
  simdata->using = 1;
  simdata->sender = NULL;

  return task;
}

/** \ingroup m_task_management
 * \brief Return the user data of a #m_task_t.
 *
 * This functions checks whether \a task is a valid pointer or not and return
   the user data associated to \a task if it is possible.
 */
void *MSG_task_get_data(m_task_t task)
{
  xbt_assert0((task != NULL), "Invalid parameter");

  return (task->data);
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
 * \brief Return the name of a #m_task_t.
 *
 * This functions returns the name of a #m_task_t as specified on creation
 */
const char *MSG_task_get_name(m_task_t task)
{
   xbt_assert0(task, "Invalid parameters");
   return task->name;
}


/** \ingroup m_task_management
 * \brief Destroy a #m_task_t.
 *
 * Destructor for #m_task_t. Note that you should free user data, if any, \b 
   before calling this function.
 */
MSG_error_t MSG_task_destroy(m_task_t task)
{
  simdata_task_t simdata = NULL;
  surf_action_t action = NULL;
  int i;

  xbt_assert0((task != NULL), "Invalid parameter");

  task->simdata->using--;
  if(task->simdata->using>0) return MSG_OK;

  xbt_assert0((xbt_dynar_length(task->simdata->sleeping)==0), 
	      "Task still used. There is a problem. Cannot destroy it now!");

  if(task->name) free(task->name);

  xbt_dynar_free(&(task->simdata->sleeping));

  action = task->simdata->compute;
  if(action) action->resource_type->common_public->action_free(action);
  action = task->simdata->comm;
  if(action) action->resource_type->common_public->action_free(action);
  if(task->simdata->host_list) xbt_free(task->simdata->host_list);

  free(task->simdata);
  free(task);

  return MSG_OK;
}

/** \ingroup m_task_management
 * \brief Returns the computation amount needed to process a task #m_task_t.
 *
 */
double MSG_task_get_compute_duration(m_task_t task)
{
  simdata_task_t simdata = NULL;

  xbt_assert0((task != NULL) && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->computation_amount;
}

/** \ingroup m_task_management
 * \brief Returns the size of the data attached to a task #m_task_t.
 *
 */
double MSG_task_get_data_size(m_task_t task)
{
  simdata_task_t simdata = NULL;

  xbt_assert0((task != NULL) && (task->simdata != NULL), "Invalid parameter");

  return task->simdata->message_size;
}

/* static MSG_error_t __MSG_task_check(m_task_t task) */
/* { */
/*   simdata_task_t simdata = NULL; */
/*   int warning = 0; */

/*   if (task == NULL) {		/\* Fatal *\/ */
/*     WARNING("Task uninitialized"); */
/*     return MSG_FATAL; */
/*   } */
/*   simdata = task->simdata; */

/*   if (simdata == NULL) {	/\* Fatal *\/ */
/*     WARNING("Simulator Data uninitialized"); */
/*     return MSG_FATAL; */
/*   } */

/*   if (simdata->compute == NULL) {	/\* Fatal if execute ... *\/ */
/*     WARNING("No duration set for this task"); */
/*     warning++; */
/*   } */

/*   if (simdata->message_size == 0) {	/\* Fatal if transfered ... *\/ */
/*     WARNING("No message_size set for this task"); */
/*     warning++; */
/*   } */

/* /\*    if (task->data == NULL) { *\/ */
/* /\*      WARNING("User Data uninitialized"); *\/ */
/* /\*      warning++; *\/ */
/* /\*    } *\/ */

/*   if (warning) */
/*     return MSG_WARNING; */
/*   return MSG_OK; */
/* } */

/* static m_task_t __MSG_task_copy(m_task_t src) */
/* { */
/*   m_task_t copy = NULL; */
/*   simdata_task_t simdata = NULL; */

/*   __MSG_task_check(src); */

/*   simdata = src->simdata; */
/*   copy = MSG_task_create(src->name, SG_getTaskCost(simdata->compute), */
/* 			 simdata->message_size, MSG_task_get_data(src)); */

/*   return (copy); */
/* } */

MSG_error_t __MSG_task_wait_event(m_process_t process, m_task_t task)
{
  int _cursor;
  m_process_t proc = NULL;

  xbt_assert0(((task != NULL)
	       && (task->simdata != NULL)), "Invalid parameters");

  xbt_dynar_push(task->simdata->sleeping, &process);
  process->simdata->waiting_task = task;
  xbt_context_yield();
  process->simdata->waiting_task = NULL;
  xbt_dynar_foreach(task->simdata->sleeping,_cursor,proc) {
    if(proc==process) 
      xbt_dynar_remove_at(task->simdata->sleeping,_cursor,&proc);
  }

  return MSG_OK;
}
