/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(m_process, msg,
				"Logging specific to MSG (process)");

/******************************** Process ************************************/
/** \ingroup m_process_management
 * \brief Creates and runs a new #m_process_t.

 * A constructor for #m_process_t taking four arguments and returning the 
 * corresponding object. The structure (and the corresponding thread) is
 * created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information
   and can be NULL.
 * \param code is a function describing the behavior of the agent. It
   should then only use functions described in \ref
   m_process_management (to create a new #m_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose
   name contains the word get), in \ref m_task_management (to create
   or destroy some #m_task_t for example) and in \ref
   msg_gos_functions (to handle file transfers and task processing).
 * \param data a pointer to any data may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new agent is executed.
 * \see m_process_t
 * \return The new corresponding object.
 */
m_process_t MSG_process_create(const char *name,
			       m_process_code_t code, void *data,
			       m_host_t host)
{
  return MSG_process_create_with_arguments(name, code, data, host, -1, NULL);
}

static void MSG_process_cleanup(void *arg)
{
  xbt_fifo_remove(msg_global->process_list, arg);
  xbt_fifo_remove(msg_global->process_to_run, arg);
  xbt_fifo_remove(((m_process_t) arg)->simdata->host->simdata->process_list, arg);
  xbt_free(((m_process_t) arg)->name);
  xbt_free(((m_process_t) arg)->simdata);
  xbt_free(arg);
}


m_process_t MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      m_host_t host, int argc, char **argv)
{
  simdata_process_t simdata = xbt_new0(s_simdata_process_t,1);
  m_process_t process = xbt_new0(s_m_process_t,1);
  m_process_t self = NULL;
  static int PID = 1;

  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");
  /* Simulator Data */

  simdata->PID = PID++;
  simdata->host = host;
  simdata->waiting_task = NULL;
  simdata->argc = argc;
  simdata->argv = argv;
  simdata->context = xbt_context_new(code, NULL, NULL, 
				     MSG_process_cleanup, process, 
				     simdata->argc, simdata->argv);

  if((self=msg_global->current_process)) {
    simdata->PPID = MSG_process_get_PID(self);
  } else {
    simdata->PPID = -1;
  }
  simdata->last_errno=MSG_OK;


  /* Process structure */
  process->name = xbt_strdup(name);
  process->simdata = simdata;
  process->data = data;

  xbt_fifo_push(host->simdata->process_list, process);

  /* /////////////// FIX du current_process !!! ////////////// */
  self = msg_global->current_process;
  xbt_context_start(process->simdata->context);
  msg_global->current_process = self;

  xbt_fifo_push(msg_global->process_list, process);
  xbt_fifo_push(msg_global->process_to_run, process);

  return process;
}

/** \ingroup m_process_management
 * \brief Migrates an agent to another location.
 *
 * This functions checks whether \a process and \a host are valid pointers
   and change the value of the #m_host_t on which \a process is running.
 */
MSG_error_t MSG_process_change_host(m_process_t process, m_host_t host)
{
  simdata_process_t simdata = NULL;

  /* Sanity check */

  xbt_assert0(((process) && (process->simdata)
	  && (host)), "Invalid parameters");
  simdata = process->simdata;

  xbt_fifo_remove(simdata->host->simdata->process_list,process);
  simdata->host = host;
  xbt_fifo_push(host->simdata->process_list,process);

  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Return the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return the user data associated to \a process if it is possible.
 */
void *MSG_process_get_data(m_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  return (process->data);
}

/** \ingroup m_process_management
 * \brief Set the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and set the user data associated to \a process if it is possible.
 */
MSG_error_t MSG_process_set_data(m_process_t process,void *data)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  xbt_assert0((process->data == NULL), "Data already set");
  
  process->data = data;
   
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Return the location on which an agent is running.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return the m_host_t corresponding to the location on which \a 
   process is running.
 */
m_host_t MSG_process_get_host(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->host);
}

/** \ingroup m_process_management
 *
 * \brief Return a #m_process_t given its PID.
 *
 * This functions search in the list of all the created m_process_t for a m_process_t 
   whose PID is equal to \a PID. If no host is found, \c NULL is returned. 
   Note that the PID are uniq in the whole simulation, not only on a given host.
 */
m_process_t MSG_process_from_PID(int PID)
{
  xbt_fifo_item_t i = NULL;
  m_process_t process = NULL;

  xbt_fifo_foreach(msg_global->process_list,i,process,m_process_t) {
    if(MSG_process_get_PID(process) == PID) return process;
  }
  return NULL;
}

/** \ingroup m_process_management
 * \brief Returns the process ID of \a process.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its PID.
 */
int MSG_process_get_PID(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->PID);
}


/** \ingroup m_process_management
 * \brief Returns the process ID of the parent of \a process.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its PID. Returns -1 if the agent has not been created by 
   another agent.
 */
int MSG_process_get_PPID(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (((simdata_process_t) process->simdata)->PPID);
}

/** \ingroup m_process_management
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not 
   and return its name.
 */
const char *MSG_process_get_name(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->name);
}

/** \ingroup m_process_management
 * \brief Return the PID of the current agent.
 *
 * This functions returns the PID of the currently running #m_process_t.
 */
int MSG_process_self_PID(void)
{
  return (MSG_process_get_PID(MSG_process_self()));
}

/** \ingroup m_process_management
 * \brief Return the PPID of the current agent.
 *
 * This functions returns the PID of the parent of the currently
 * running #m_process_t.
 */
int MSG_process_self_PPID(void)
{
  return (MSG_process_get_PPID(MSG_process_self()));
}

/** \ingroup m_process_management
 * \brief Return the current agent.
 *
 * This functions returns the currently running #m_process_t.
 */
m_process_t MSG_process_self(void)
{
  return msg_global->current_process;
}

/** \ingroup m_process_management
 * \brief Suspend the process.
 *
 * This functions suspend the process by suspending the task on which
 * it was waiting for the completion.
 */
MSG_error_t MSG_process_suspend(m_process_t process)
{
  simdata_process_t simdata = NULL;
  simdata_task_t simdata_task = NULL;
  int i;

  xbt_assert0(((process) && (process->simdata)), "Invalid parameters");

  if(process!=MSG_process_self()) {
    simdata = process->simdata;
    
    xbt_assert0(simdata->waiting_task,"Process not waiting for anything else. Weird !");

    simdata_task = simdata->waiting_task->simdata;

    xbt_assert0(((simdata_task->compute)||(simdata_task->comm))&&
		!((simdata_task->comm)&&(simdata_task->comm)),
		"Got a problem in deciding which action to choose !");
    simdata->suspended = 1;
    if(simdata_task->compute) 
      surf_workstation_resource->extension_public->suspend(simdata_task->compute);
    else
      surf_workstation_resource->extension_public->suspend(simdata_task->comm);
  } else {
    m_task_t dummy = MSG_TASK_UNINITIALIZED;
    dummy = MSG_task_create("suspended", 0.0, 0, NULL);

    simdata->suspended = 1;
    __MSG_task_execute(process,dummy);
    surf_workstation_resource->extension_public->suspend(dummy->simdata->compute);
    __MSG_wait_for_computation(process,dummy);
    simdata->suspended = 0;

    MSG_task_destroy(dummy);
  }
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Resume a suspended process.
 *
 * This functions resume a suspended process by resuming the task on
 * which it was waiting for the completion.
 */
MSG_error_t MSG_process_resume(m_process_t process)
{
  simdata_process_t simdata = NULL;
  simdata_task_t simdata_task = NULL;
  int i;

  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");
  CHECK_HOST();

  simdata = process->simdata;

  if(simdata->blocked) {
    simdata->suspended = 0; /* He'll wake up by itself */
    MSG_RETURN(MSG_OK);
  }

  if(!(simdata->waiting_task)) {
    xbt_assert0(0,"Process not waiting for anything else. Weird !");
    return MSG_WARNING;
  }
  simdata_task = simdata->waiting_task->simdata;


  if(simdata_task->compute) 
    surf_workstation_resource->extension_public->resume(simdata_task->compute);
  else 
    surf_workstation_resource->extension_public->resume(simdata_task->comm);

  MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the
 * task on which it was waiting for the completion.
 */
int MSG_process_isSuspended(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->suspended);
}





MSG_error_t __MSG_process_block()
{
  m_process_t process = MSG_process_self();

  m_task_t dummy = MSG_TASK_UNINITIALIZED;
  dummy = MSG_task_create("blocked", 0.0, 0, NULL);
  
  process->simdata->blocked=1;
  __MSG_task_execute(process,dummy);
  surf_workstation_resource->extension_public->suspend(dummy->simdata->compute);
  __MSG_wait_for_computation(process,dummy);
  process->simdata->blocked=0;

  if(process->simdata->suspended)
    MSG_process_suspend(process);
  
  MSG_task_destroy(dummy);

  return MSG_OK;
}

MSG_error_t __MSG_process_unblock(m_process_t process)
{
  simdata_process_t simdata = NULL;
  simdata_task_t simdata_task = NULL;
  int i;

  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");
  CHECK_HOST();

  simdata = process->simdata;
  if(!(simdata->waiting_task)) {
    xbt_assert0(0,"Process not waiting for anything else. Weird !");
    return MSG_WARNING;
  }
  simdata_task = simdata->waiting_task->simdata;

  xbt_assert0(simdata->blocked,"Process not blocked");

  surf_workstation_resource->extension_public->resume(simdata_task->compute);

  MSG_RETURN(MSG_OK);
}

int __MSG_process_isBlocked(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->blocked);
}
