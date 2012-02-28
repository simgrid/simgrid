/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_process, msg,
                                "Logging specific to MSG (process)");

/** \defgroup m_process_management Management Functions of Agents
 *  \brief This section describes the agent structure of MSG
 *  (#m_process_t) and the functions for managing it.
 */
/** @addtogroup m_process_management
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Agents" --> \endhtmlonly
 *
 *  We need to simulate many independent scheduling decisions, so
 *  the concept of <em>process</em> is at the heart of the
 *  simulator. A process may be defined as a <em>code</em>, with
 *  some <em>private data</em>, executing in a <em>location</em>.
 *  \see m_process_t
 */

/******************************** Process ************************************/

/**
 * \brief Cleans the MSG data of a process.
 * \param smx_proc a SIMIX process
 */
void MSG_process_cleanup_from_SIMIX(smx_process_t smx_proc)
{
  simdata_process_t msg_proc;

  // get the MSG process from the SIMIX process
  if (smx_proc == SIMIX_process_self()) {
    /* avoid a SIMIX request if this function is called by the process itself */
    msg_proc = SIMIX_process_self_get_data(smx_proc);
    SIMIX_process_self_set_data(smx_proc, NULL);
  }
  else {
    msg_proc = simcall_process_get_data(smx_proc);
    simcall_process_set_data(smx_proc, NULL);
  }

#ifdef HAVE_TRACING
  TRACE_msg_process_end(smx_proc);
#endif

  // free the data if a function was provided
  if (msg_proc->data && msg_global->process_data_cleanup) {
    msg_global->process_data_cleanup(msg_proc->data);
  }

  // free the MSG process
  xbt_free(msg_proc);
}

/* This function creates a MSG process. It has the prototype enforced by SIMIX_function_register_process_create */
void MSG_process_create_from_SIMIX(smx_process_t* process, const char *name,
                                    xbt_main_func_t code, void *data,
                                    const char *hostname, int argc, char **argv,
                                    xbt_dict_t properties)
{
  m_host_t host = MSG_get_host_by_name(hostname);
  m_process_t p = MSG_process_create_with_environment(name, code, data,
                                                      host, argc, argv,
                                                      properties);
  *((m_process_t*) process) = p;
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #m_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without
   providing standard arguments (\a argc, \a argv, \a start_time, \a kill_time).
 * \sa MSG_process_create_with_arguments
 */
m_process_t MSG_process_create(const char *name,
                               xbt_main_func_t code, void *data,
                               m_host_t host)
{
  return MSG_process_create_with_environment(name, code, data, host, -1,
                                             NULL, NULL);
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
                                              xbt_main_func_t code,
                                              void *data, m_host_t host,
                                              int argc, char **argv)
{
  return MSG_process_create_with_environment(name, code, data, host,
                                             argc, argv, NULL);
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
 * \param properties list a properties defined for this process
 * \see m_process_t
 * \return The new corresponding object.
 */
m_process_t MSG_process_create_with_environment(const char *name,
                                                xbt_main_func_t code,
                                                void *data, m_host_t host,
                                                int argc, char **argv,
                                                xbt_dict_t properties)
{
  xbt_assert(code != NULL && host != NULL, "Invalid parameters");
  simdata_process_t simdata = xbt_new0(s_simdata_process_t, 1);
  m_process_t process;

  /* Simulator data for MSG */
  simdata->PID = msg_global->PID++;
  simdata->waiting_action = NULL;
  simdata->waiting_task = NULL;
  simdata->m_host = host;
  simdata->argc = argc;
  simdata->argv = argv;
  simdata->data = data;
  simdata->last_errno = MSG_OK;

  if (SIMIX_process_self()) {
    simdata->PPID = MSG_process_get_PID(MSG_process_self());
  } else {
    simdata->PPID = -1;
  }

#ifdef HAVE_TRACING
  TRACE_msg_process_create(name, simdata->PID, simdata->m_host);
#endif

  /* Let's create the process: SIMIX may decide to start it right now,
   * even before returning the flow control to us */
  simcall_process_create(&process, name, code, simdata, host->name,
                           argc, argv, properties);

  if (!process) {
    /* Undo everything we have just changed */
    msg_global->PID--;
    xbt_free(simdata);
    return NULL;
  }

  return process;
}

void MSG_process_kill_from_SIMIX(smx_process_t p)
{
#ifdef HAVE_TRACING
  TRACE_msg_process_kill(p);
#endif
  MSG_process_kill(p);
}

/** \ingroup m_process_management
 * \param process poor victim
 *
 * This function simply kills a \a process... scary isn't it ? :)
 */
void MSG_process_kill(m_process_t process)
{
#ifdef HAVE_TRACING
  TRACE_msg_process_kill(process);
#endif

  /* FIXME: why do we only cancel communication actions? is this useful? */
  simdata_process_t p_simdata = simcall_process_get_data(process);
  if (p_simdata->waiting_task && p_simdata->waiting_task->simdata->comm) {
    simcall_comm_cancel(p_simdata->waiting_task->simdata->comm);
  }
 
  simcall_process_kill(process);

  return;
}

/** \ingroup m_process_management
 * \brief Migrates an agent to another location.
 *
 * This function checks whether \a process and \a host are valid pointers
   and change the value of the #m_host_t on which \a process is running.
 */
MSG_error_t MSG_process_migrate(m_process_t process, m_host_t host)
{
  simdata_process_t simdata = simcall_process_get_data(process);
  simdata->m_host = host;
#ifdef HAVE_TRACING
  m_host_t now = simdata->m_host;
  TRACE_msg_process_change_host(process, now, host);
#endif
  simcall_process_change_host(process, host->simdata->smx_host);
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Returns the user data of a process.
 *
 * This function checks whether \a process is a valid pointer or not
   and returns the user data associated to this process.
 */
void* MSG_process_get_data(m_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter");

  /* get from SIMIX the MSG process data, and then the user data */
  simdata_process_t simdata = simcall_process_get_data(process);
  return simdata->data;
}

/** \ingroup m_process_management
 * \brief Sets the user data of a process.
 *
 * This function checks whether \a process is a valid pointer or not
   and sets the user data associated to this process.
 */
MSG_error_t MSG_process_set_data(m_process_t process, void *data)
{
  xbt_assert(process != NULL, "Invalid parameter");

  simdata_process_t simdata = simcall_process_get_data(process);
  simdata->data = data;

  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Sets a cleanup function to be called to free the userdata of a
 * process when a process is destroyed.
 * \param data_cleanup a cleanup function for the userdata of a process,
 * or NULL to call no function
 */
XBT_PUBLIC(void) MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup) {

  msg_global->process_data_cleanup = data_cleanup;
}

/** \ingroup m_process_management
 * \brief Return the location on which an agent is running.
 * \param process a process (NULL means the current one)
 * \return the m_host_t corresponding to the location on which \a
 * process is running.
 */
m_host_t MSG_process_get_host(m_process_t process)
{
  simdata_process_t simdata;
  if (process == NULL) {
    simdata = SIMIX_process_self_get_data(SIMIX_process_self());
  }
  else {
    simdata = simcall_process_get_data(process);
  }
  return simdata->m_host;
}

/** \ingroup m_process_management
 *
 * \brief Return a #m_process_t given its PID.
 *
 * This function search in the list of all the created m_process_t for a m_process_t
   whose PID is equal to \a PID. If no host is found, \c NULL is returned.
   Note that the PID are uniq in the whole simulation, not only on a given host.
 */
m_process_t MSG_process_from_PID(int PID)
{
	return SIMIX_process_from_PID(PID);
}

/** @brief returns a list of all currently existing processes */
xbt_dynar_t MSG_processes_as_dynar(void) {
  return SIMIX_processes_as_dynar();
}

/** \ingroup m_process_management
 * \brief Returns the process ID of \a process.
 *
 * This function checks whether \a process is a valid pointer or not
   and return its PID (or 0 in case of problem).
 */
int MSG_process_get_PID(m_process_t process)
{
  /* Do not raise an exception here: this function is called by the logs
   * and the exceptions, so it would be called back again and again */
  if (process == NULL) {
    return 0;
  }

  simdata_process_t simdata = simcall_process_get_data(process);

  return simdata != NULL ? simdata->PID : 0;
}

/** \ingroup m_process_management
 * \brief Returns the process ID of the parent of \a process.
 *
 * This function checks whether \a process is a valid pointer or not
   and return its PID. Returns -1 if the agent has not been created by
   another agent.
 */
int MSG_process_get_PPID(m_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter");

  simdata_process_t simdata = simcall_process_get_data(process);

  return simdata->PPID;
}

/** \ingroup m_process_management
 * \brief Return the name of an agent.
 *
 * This function checks whether \a process is a valid pointer or not
   and return its name.
 */
const char *MSG_process_get_name(m_process_t process)
{
  xbt_assert(process, "Invalid parameter");

  return simcall_process_get_name(process);
}

/** \ingroup m_process_management
 * \brief Returns the value of a given process property
 *
 * \param process a process
 * \param name a property name
 * \return value of a property (or NULL if the property is not set)
 */
const char *MSG_process_get_property_value(m_process_t process,
                                           const char *name)
{
  return xbt_dict_get_or_null(MSG_process_get_properties(process), name);
}

/** \ingroup m_process_management
 * \brief Return the list of properties
 *
 * This function returns all the parameters associated with a process
 */
xbt_dict_t MSG_process_get_properties(m_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter");

  return simcall_process_get_properties(process);

}

/** \ingroup m_process_management
 * \brief Return the PID of the current agent.
 *
 * This function returns the PID of the currently running #m_process_t.
 */
int MSG_process_self_PID(void)
{
  return MSG_process_get_PID(MSG_process_self());
}

/** \ingroup m_process_management
 * \brief Return the PPID of the current agent.
 *
 * This function returns the PID of the parent of the currently
 * running #m_process_t.
 */
int MSG_process_self_PPID(void)
{
  return MSG_process_get_PPID(MSG_process_self());
}

/** \ingroup m_process_management
 * \brief Return the current process.
 *
 * This function returns the currently running #m_process_t.
 */
m_process_t MSG_process_self(void)
{
  return SIMIX_process_self();
}

/** \ingroup m_process_management
 * \brief Suspend the process.
 *
 * This function suspends the process by suspending the task on which
 * it was waiting for the completion.
 */
MSG_error_t MSG_process_suspend(m_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter");
  CHECK_HOST();

#ifdef HAVE_TRACING
  TRACE_msg_process_suspend(process);
#endif

  simcall_process_suspend(process);
  MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Resume a suspended process.
 *
 * This function resumes a suspended process by resuming the task on
 * which it was waiting for the completion.
 */
MSG_error_t MSG_process_resume(m_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter");
  CHECK_HOST();

#ifdef HAVE_TRACING
  TRACE_msg_process_resume(process);
#endif

  simcall_process_resume(process);
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
  xbt_assert(process != NULL, "Invalid parameter");
  return simcall_process_is_suspended(process);
}

smx_context_t MSG_process_get_smx_ctx(m_process_t process) {
  return SIMIX_process_get_context(process);
}
