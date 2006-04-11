/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(m_process, msg,
				"Logging specific to MSG (process)");

/** \defgroup m_process_management Management Functions of Agents
 *  \brief This section describes the agent structure of MSG
 *  (#m_process_t) and the functions for managing it.
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Agents" --> \endhtmlonly
 * 
 *  We need to simulate many independent scheduling decisions, so
 *  the concept of <em>process</em> is at the heart of the
 *  simulator. A process may be defined as a <em>code</em>, with
 *  some <em>private data</em>, executing in a <em>location</em>.
 *  \see m_process_t
 */

/******************************** Process ************************************/
/** \ingroup m_process_management
 * \brief Creates and runs a new #m_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without 
   providing standard arguments (\a argc, \a argv, \a start_time, \a kill_time).
 * \sa MSG_process_create_with_arguments
 */
m_process_t MSG_process_create(const char *name,
			       m_process_code_t code, void *data,
			       m_host_t host)
{
  return MSG_process_create_with_arguments(name, code, data, host, -1, NULL);
}

static void MSG_process_cleanup(void *arg)
{

  while(((m_process_t)arg)->simdata->paje_state) {
    PAJE_PROCESS_POP_STATE((m_process_t)arg);
  }

  PAJE_PROCESS_FREE(arg);

  xbt_fifo_remove(msg_global->process_list, arg);
  xbt_fifo_remove(msg_global->process_to_run, arg);
  xbt_fifo_remove(((m_process_t) arg)->simdata->host->simdata->process_list, arg);
  free(((m_process_t) arg)->name);
  ((m_process_t) arg)->name = NULL;
  free(((m_process_t) arg)->simdata);
  ((m_process_t) arg)->simdata = NULL;
  free(arg);
}

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
 * \param data a pointer to any data one may want to attach to the new
   object.  It is for user-level information and can be NULL. It can
   be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new agent is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \see m_process_t
 * \return The new corresponding object.
 */
m_process_t MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      m_host_t host, int argc, char **argv)
{
  simdata_process_t simdata = xbt_new0(s_simdata_process_t,1);
  m_process_t process = xbt_new0(s_m_process_t,1);
  m_process_t self = NULL;

  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");
  /* Simulator Data */

  simdata->PID = msg_global->PID++;
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

  xbt_fifo_unshift(host->simdata->process_list, process);

  /* /////////////// FIX du current_process !!! ////////////// */
  self = msg_global->current_process;
  xbt_context_start(process->simdata->context);
  msg_global->current_process = self;

  xbt_fifo_unshift(msg_global->process_list, process);
  DEBUG2("Inserting %s(%s) in the to_run list",process->name,
	 host->name);
  xbt_fifo_unshift(msg_global->process_to_run, process);

  PAJE_PROCESS_NEW(process);

  return process;
}

/** \ingroup m_process_management
 * \param process poor victim
 *
 * This function simply kills a \a process... scarry isn't it ? :)
 */
void MSG_process_kill(m_process_t process)
{
  int i;
  simdata_process_t p_simdata = process->simdata;
  simdata_host_t h_simdata= p_simdata->host->simdata;
  int _cursor;
  m_process_t proc = NULL;

/*   fprintf(stderr,"Killing %s(%d) on %s.\n",process->name, */
/* 	  p_simdata->PID,p_simdata->host->name); */
  
  for (i=0; i<msg_global->max_channel; i++) {
    if (h_simdata->sleeping[i] == process) {
      h_simdata->sleeping[i] = NULL;
      break;
    }
  }
  if (i==msg_global->max_channel) {
    if(p_simdata->waiting_task) {
      xbt_dynar_foreach(p_simdata->waiting_task->simdata->sleeping,_cursor,proc) {
	if(proc==process) 
	  xbt_dynar_remove_at(p_simdata->waiting_task->simdata->sleeping,_cursor,&proc);
      }
      if(p_simdata->waiting_task->simdata->compute)
	surf_workstation_resource->common_public->
	  action_free(p_simdata->waiting_task->simdata->compute);
      else if (p_simdata->waiting_task->simdata->comm) {
	surf_workstation_resource->common_public->
	  action_change_state(p_simdata->waiting_task->simdata->comm,SURF_ACTION_FAILED);
	surf_workstation_resource->common_public->
	  action_free(p_simdata->waiting_task->simdata->comm);
      } else 
	CRITICAL0("UNKNOWN STATUS. Please report this bug.");
    } else { /* Must be trying to put a task somewhere */
      if(process==MSG_process_self()) {
	return;
      } else {
	CRITICAL0("UNKNOWN STATUS. Please report this bug.");
      }
    }
  }

  xbt_fifo_remove(msg_global->process_to_run,process);
  xbt_fifo_remove(msg_global->process_list,process);
  xbt_context_free(process->simdata->context);
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
  xbt_fifo_unshift(host->simdata->process_list,process);

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
  return msg_global ? msg_global->current_process : NULL;
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

  XBT_IN2("(%p(%s))", process, process->name);

  xbt_assert0(((process) && (process->simdata)), "Invalid parameters");

  PAJE_PROCESS_PUSH_STATE(process,"S");

  if(process!=MSG_process_self()) {
    simdata = process->simdata;
    
    xbt_assert0(simdata->waiting_task,"Process not waiting for anything else. Weird !");

    simdata_task = simdata->waiting_task->simdata;

    simdata->suspended = 1;
    if(simdata->blocked) {
      XBT_OUT;
      return MSG_OK;
    }

    xbt_assert0(((simdata_task->compute)||(simdata_task->comm))&&
		!((simdata_task->compute)&&(simdata_task->comm)),
		"Got a problem in deciding which action to choose !");
    simdata->suspended = 1;
    if(simdata_task->compute) 
      surf_workstation_resource->common_public->suspend(simdata_task->compute);
    else
      surf_workstation_resource->common_public->suspend(simdata_task->comm);
  } else {
    m_task_t dummy = MSG_TASK_UNINITIALIZED;
    dummy = MSG_task_create("suspended", 0.0, 0, NULL);

    simdata = process->simdata;
    simdata->suspended = 1;
    __MSG_task_execute(process,dummy);
    surf_workstation_resource->common_public->suspend(dummy->simdata->compute);
    __MSG_wait_for_computation(process,dummy);
    simdata->suspended = 0;

    MSG_task_destroy(dummy);
  }
  XBT_OUT;
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

  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");
  CHECK_HOST();

  XBT_IN2("(%p(%s))", process, process->name);

  if(process == MSG_process_self()) {
    XBT_OUT;
    MSG_RETURN(MSG_OK);
  }

  simdata = process->simdata;

  if(simdata->blocked) {
    PAJE_PROCESS_POP_STATE(process);

    simdata->suspended = 0; /* He'll wake up by itself */
    XBT_OUT;
    MSG_RETURN(MSG_OK);
  }

  if(!(simdata->waiting_task)) {
    xbt_assert0(0,"Process not waiting for anything else. Weird !");
    XBT_OUT;
    return MSG_WARNING;
  }
  simdata_task = simdata->waiting_task->simdata;


  if(simdata_task->compute) {
    surf_workstation_resource->common_public->resume(simdata_task->compute);
    PAJE_PROCESS_POP_STATE(process);
  }
  else {
    PAJE_PROCESS_POP_STATE(process);
    surf_workstation_resource->common_public->resume(simdata_task->comm);
  }

  XBT_OUT;
  MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the
 * task on which it was waiting for the completion.
 */
int MSG_process_is_suspended(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->suspended);
}

static char blocked_name[512];

int __MSG_process_block(double max_duration)
{
  m_process_t process = MSG_process_self();

  m_task_t dummy = MSG_TASK_UNINITIALIZED;
  snprintf(blocked_name,512,"blocked (%s:%s)",process->name,
	  process->simdata->host->name);

  XBT_IN1(": max_duration=%g",max_duration);

  dummy = MSG_task_create(blocked_name, 0.0, 0, NULL);
  
  PAJE_PROCESS_PUSH_STATE(process,"B");

  process->simdata->blocked=1;
  __MSG_task_execute(process,dummy);
  surf_workstation_resource->common_public->suspend(dummy->simdata->compute);
  if(max_duration>=0)
    surf_workstation_resource->common_public->set_max_duration(dummy->simdata->compute, 
							       max_duration);
  __MSG_wait_for_computation(process,dummy);
  MSG_task_destroy(dummy);
  process->simdata->blocked=0;

  if(process->simdata->suspended) {
    DEBUG0("I've been suspended in the meantime");    
    MSG_process_suspend(process);
    DEBUG0("I've been resumed, let's keep going");    
  }

  XBT_OUT;
  return 1;
}

MSG_error_t __MSG_process_unblock(m_process_t process)
{
  simdata_process_t simdata = NULL;
  simdata_task_t simdata_task = NULL;

  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");
  CHECK_HOST();

  XBT_IN2(": %s unblocking %s", MSG_process_self()->name,process->name);

  simdata = process->simdata;
  if(!(simdata->waiting_task)) {
    xbt_assert0(0,"Process not waiting for anything else. Weird !");
    XBT_OUT;
    return MSG_WARNING;
  }
  simdata_task = simdata->waiting_task->simdata;

  xbt_assert0(simdata->blocked,"Process not blocked");

  surf_workstation_resource->common_public->resume(simdata_task->compute);

  PAJE_PROCESS_POP_STATE(process);

  XBT_OUT;

  MSG_RETURN(MSG_OK);
}

int __MSG_process_isBlocked(m_process_t process)
{
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->blocked);
}
