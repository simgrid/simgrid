/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "src/simix/smx_process_private.h"
#include "src/simix/smx_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_process, msg, "Logging specific to MSG (process)");

/** @addtogroup m_process_management
 * \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Processes" --> \endhtmlonly
 *
 *  Processes (#msg_process_t) are independent agents that can do stuff on their own. They are in charge of executing
 *  your code interacting with the simulated world.
 *  A process may be defined as a <em>code</em> with some <em>private data</em>.
 *  Processes must be located on <em>hosts</em> (#msg_host_t), and they exchange data by sending tasks (#msg_task_t)
 *  that are similar to envelops containing data.
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
    msg_proc = (simdata_process_t) SIMIX_process_self_get_data();
    SIMIX_process_self_set_data(NULL);
  } else {
    msg_proc = (simdata_process_t) simcall_process_get_data(smx_proc);
    simcall_process_set_data(smx_proc, NULL);
  }

  TRACE_msg_process_destroy(smx_proc->name, smx_proc->pid);
  // free the data if a function was provided
  if (msg_proc && msg_proc->data && msg_global->process_data_cleanup) {
    msg_global->process_data_cleanup(msg_proc->data);
  }

  // free the MSG process
  xbt_free(msg_proc);
  SIMIX_process_cleanup(smx_proc);
}

/* This function creates a MSG process. It has the prototype enforced by SIMIX_function_register_process_create */
smx_process_t MSG_process_create_from_SIMIX(const char *name, xbt_main_func_t code, void *data, const char *hostname,
                                            double kill_time, int argc, char **argv, xbt_dict_t properties,
                                            int auto_restart, smx_process_t parent_process)
{
  msg_host_t host = MSG_host_by_name(hostname);
  msg_process_t p = MSG_process_create_with_environment(name, code, data, host, argc, argv, properties);
  if (p) {
    MSG_process_set_kill_time(p,kill_time);
    MSG_process_auto_restart_set(p,auto_restart);
  }
  return p;
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #msg_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without providing standard arguments
 * (\a argc, \a argv, \a start_time, \a kill_time).
 * \sa MSG_process_create_with_arguments
 */
msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
{
  return MSG_process_create_with_environment(name, code, data, host, 0, NULL, NULL);
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param code is a function describing the behavior of the process. It should then only use functions described
 * in \ref m_process_management (to create a new #msg_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in \ref m_task_management (to create or destroy some #msg_task_t for example) and
   in \ref msg_task_usage (to handle file transfers and task processing).
 * \param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be NULL. It can be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new process is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \see msg_process_t
 * \return The new corresponding object.
 */

msg_process_t MSG_process_create_with_arguments(const char *name, xbt_main_func_t code, void *data, msg_host_t host,
                                              int argc, char **argv)
{
  return MSG_process_create_with_environment(name, code, data, host, argc, argv, NULL);
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param code is a function describing the behavior of the process. It should then only use functions described
 * in \ref m_process_management (to create a new #msg_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in \ref m_task_management (to create or destroy some #msg_task_t for example) and
   in \ref msg_task_usage (to handle file transfers and task processing).
 * \param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be NULL. It can be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new process is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code. WARNING, these strings are freed by the SimGrid kernel when the
 *             process exits, so they cannot be static nor shared between several processes.
 * \param properties list a properties defined for this process
 * \see msg_process_t
 * \return The new corresponding object.
 */
msg_process_t MSG_process_create_with_environment(const char *name, xbt_main_func_t code, void *data, msg_host_t host,
                                                int argc, char **argv, xbt_dict_t properties)
{
  xbt_assert(code != NULL && host != NULL, "Invalid parameters: host and code params must not be NULL");
  simdata_process_t simdata = xbt_new0(s_simdata_process_t, 1);
  msg_process_t process;

  /* Simulator data for MSG */
  simdata->waiting_action = NULL;
  simdata->waiting_task = NULL;
  simdata->m_host = host;
  simdata->argc = argc;
  simdata->argv = argv;
  simdata->data = data;
  simdata->last_errno = MSG_OK;

  /* Let's create the process: SIMIX may decide to start it right now,
   * even before returning the flow control to us */
  process = simcall_process_create(name, code, simdata, sg_host_get_name(host), -1, argc, argv, properties,0);

  if (!process) {
    /* Undo everything we have just changed */
    xbt_free(simdata);
    return NULL;
  }
  else {
    simcall_process_on_exit(process,(int_f_pvoid_pvoid_t)TRACE_msg_process_kill,process);
  }
  return process;
}

static int MSG_maestro(int argc, char** argv)
{
  int res = MSG_main();
  return res;
}

/* Become a process in the simulation
 *
 * Currently this can only be called by the main thread (once) and only work with some thread factories
 * (currently ThreadContextFactory).
 *
 * In the future, it might be extended in order to attach other threads created by a third party library.
 */
msg_process_t MSG_process_attach(const char *name, void *data, msg_host_t host, xbt_dict_t properties)
{
  xbt_assert(host != NULL, "Invalid parameters: host and code params must not be NULL");
  simdata_process_t simdata = xbt_new0(s_simdata_process_t, 1);
  msg_process_t process;

  /* Simulator data for MSG */
  simdata->waiting_action = NULL;
  simdata->waiting_task = NULL;
  simdata->m_host = host;
  simdata->argc = 0;
  simdata->argv = NULL;
  simdata->data = data;
  simdata->last_errno = MSG_OK;

  /* Let's create the process: SIMIX may decide to start it right now, even before returning the flow control to us */
  process = SIMIX_process_attach(name, simdata, sg_host_get_name(host), properties, NULL);
  if (!process)
    xbt_die("Could not attach");
  simcall_process_on_exit(process,(int_f_pvoid_pvoid_t)TRACE_msg_process_kill,process);
  return process;
}

/** Detach a process attached with `MSG_process_attach()`
 *
 *  This is called when the current process has finished its job.
 *  Used in the main thread, it waits for the simulation to finish before  returning. When it returns, the other
 *  simulated processes and the maestro are destroyed.
 */
void MSG_process_detach(void)
{
  SIMIX_process_detach();
}

/** \ingroup m_process_management
 * \param process poor victim
 *
 * This function simply kills a \a process... scary isn't it ? :)
 */
void MSG_process_kill(msg_process_t process)
{
//  /* FIXME: why do we only cancel communication actions? is this useful? */
//  simdata_process_t p_simdata = simcall_process_get_data(process);
//  if (p_simdata->waiting_task && p_simdata->waiting_task->simdata->comm) {
//    simcall_comm_cancel(p_simdata->waiting_task->simdata->comm);
//  }
  simcall_process_kill(process);
}

/**
* \brief Wait for the completion of a #msg_process_t.
*
* \param process the process to wait for
* \param timeout wait until the process is over, or the timeout occurs
*/
msg_error_t MSG_process_join(msg_process_t process, double timeout){
  simcall_process_join(process, timeout);
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Migrates a process to another location.
 *
 * This function checks whether \a process and \a host are valid pointers and change the value of the #msg_host_t on
 * which \a process is running.
 */
msg_error_t MSG_process_migrate(msg_process_t process, msg_host_t host)
{
  simdata_process_t simdata = (simdata_process_t) simcall_process_get_data(process);
  simdata->m_host = host;
  msg_host_t now = simdata->m_host;
  TRACE_msg_process_change_host(process, now, host);
  simcall_process_set_host(process, host);
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Returns the user data of a process.
 *
 * This function checks whether \a process is a valid pointer and returns the user data associated to this process.
 */
void* MSG_process_get_data(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: first parameter must not be NULL!");

  /* get from SIMIX the MSG process data, and then the user data */
  simdata_process_t simdata = (simdata_process_t) simcall_process_get_data(process);
  if (simdata)
    return simdata->data;
  else
    return NULL;
}

/** \ingroup m_process_management
 * \brief Sets the user data of a process.
 *
 * This function checks whether \a process is a valid pointer and sets the user data associated to this process.
 */
msg_error_t MSG_process_set_data(msg_process_t process, void *data)
{
  xbt_assert(process != NULL, "Invalid parameter: first parameter must not be NULL!");

  simdata_process_t simdata = (simdata_process_t) simcall_process_get_data(process);
  simdata->data = data;

  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Sets a cleanup function to be called to free the userdata of a process when a process is destroyed.
 * \param data_cleanup a cleanup function for the userdata of a process, or NULL to call no function
 */
XBT_PUBLIC(void) MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup) {
  msg_global->process_data_cleanup = data_cleanup;
}

/** \ingroup m_process_management
 * \brief Return the location on which a process is running.
 * \param process a process (NULL means the current one)
 * \return the msg_host_t corresponding to the location on which \a process is running.
 */
msg_host_t MSG_process_get_host(msg_process_t process)
{
  simdata_process_t simdata;
  if (process == NULL) {
    simdata = (simdata_process_t) SIMIX_process_self_get_data();
  }
  else {
    simdata = (simdata_process_t) simcall_process_get_data(process);
  }
  return simdata ? simdata->m_host : NULL;
}

/** \ingroup m_process_management
 *
 * \brief Return a #msg_process_t given its PID.
 *
 * This function search in the list of all the created msg_process_t for a msg_process_t  whose PID is equal to \a PID.
 * If no host is found, \c NULL is returned.
   Note that the PID are uniq in the whole simulation, not only on a given host.
 */
msg_process_t MSG_process_from_PID(int PID)
{
  return SIMIX_process_from_PID(PID);
}

/** @brief returns a list of all currently existing processes */
xbt_dynar_t MSG_processes_as_dynar(void) {
  return SIMIX_processes_as_dynar();
}

/** @brief Return the current number MSG processes. */
int MSG_process_get_number(void)
{
  return SIMIX_process_count();
}

/** \ingroup m_process_management
 * \brief Set the kill time of a process.
 *
 * \param process a process
 * \param kill_time the time when the process is killed.
 */
msg_error_t MSG_process_set_kill_time(msg_process_t process, double kill_time)
{
  simcall_process_set_kill_time(process,kill_time);
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Returns the process ID of \a process.
 *
 * This function checks whether \a process is a valid pointer and return its PID (or 0 in case of problem).
 */
int MSG_process_get_PID(msg_process_t process)
{
  /* Do not raise an exception here: this function is called by the logs
   * and the exceptions, so it would be called back again and again */
  if (process == NULL) {
    return 0;
  }
  return simcall_process_get_PID(process);
}

/** \ingroup m_process_management
 * \brief Returns the process ID of the parent of \a process.
 *
 * This function checks whether \a process is a valid pointer and return its PID.
 * Returns -1 if the process has not been created by any other process.
 */
int MSG_process_get_PPID(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");
  return simcall_process_get_PPID(process);
}

/** \ingroup m_process_management
 * \brief Return the name of a process.
 *
 * This function checks whether \a process is a valid pointer and return its name.
 */
const char *MSG_process_get_name(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");
  return simcall_process_get_name(process);
}

/** \ingroup m_process_management
 * \brief Returns the value of a given process property
 *
 * \param process a process
 * \param name a property name
 * \return value of a property (or NULL if the property is not set)
 */
const char *MSG_process_get_property_value(msg_process_t process, const char *name)
{
  return (char*) xbt_dict_get_or_null(MSG_process_get_properties(process), name);
}

/** \ingroup m_process_management
 * \brief Return the list of properties
 *
 * This function returns all the parameters associated with a process
 */
xbt_dict_t MSG_process_get_properties(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");
  return simcall_process_get_properties(process);
}

/** \ingroup m_process_management
 * \brief Return the PID of the current process.
 *
 * This function returns the PID of the currently running #msg_process_t.
 */
int MSG_process_self_PID(void)
{
  return MSG_process_get_PID(MSG_process_self());
}

/** \ingroup m_process_management
 * \brief Return the PPID of the current process.
 *
 * This function returns the PID of the parent of the currently running #msg_process_t.
 */
int MSG_process_self_PPID(void)
{
  return MSG_process_get_PPID(MSG_process_self());
}

/** \ingroup m_process_management
 * \brief Return the current process.
 *
 * This function returns the currently running #msg_process_t.
 */
msg_process_t MSG_process_self(void)
{
  return SIMIX_process_self();
}

/** \ingroup m_process_management
 * \brief Suspend the process.
 *
 * This function suspends the process by suspending the task on which it was waiting for the completion.
 */
msg_error_t MSG_process_suspend(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");

  TRACE_msg_process_suspend(process);
  simcall_process_suspend(process);
  MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Resume a suspended process.
 *
 * This function resumes a suspended process by resuming the task on which it was waiting for the completion.
 */
msg_error_t MSG_process_resume(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");

  TRACE_msg_process_resume(process);
  simcall_process_resume(process);
  MSG_RETURN(MSG_OK);
}

/** \ingroup m_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 */
int MSG_process_is_suspended(msg_process_t process)
{
  xbt_assert(process != NULL, "Invalid parameter: First argument must not be NULL");
  return simcall_process_is_suspended(process);
}

smx_context_t MSG_process_get_smx_ctx(msg_process_t process) {
  return SIMIX_process_get_context(process);
}
/**
 * \ingroup m_process_management
 * \brief Add a function to the list of "on_exit" functions for the current process.
 * The on_exit functions are the functions executed when your process is killed.
 * You should use them to free the data used by your process.
 */
void MSG_process_on_exit(int_f_pvoid_pvoid_t fun, void *data) {
  simcall_process_on_exit(MSG_process_self(),fun,data);
}
/**
 * \ingroup m_process_management
 * \brief Sets the "auto-restart" flag of the process.
 * If the flag is set to 1, the process will be automatically restarted when its host comes back up.
 */
XBT_PUBLIC(void) MSG_process_auto_restart_set(msg_process_t process, int auto_restart) {
  simcall_process_auto_restart_set(process,auto_restart);
}
/*
 * \ingroup m_process_management
 * \brief Restarts a process from the beginning.
 */
XBT_PUBLIC(msg_process_t) MSG_process_restart(msg_process_t process) {
  return simcall_process_restart(process);
}
