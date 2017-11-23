/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_process, msg, "Logging specific to MSG (process)");

extern "C" {

/** @addtogroup m_process_management
 *
 *  Processes (#msg_process_t) are independent agents that can do stuff on their own. They are in charge of executing
 *  your code interacting with the simulated world.
 *  A process may be defined as a <em>code</em> with some <em>private data</em>.
 *  Processes must be located on <em>hosts</em> (#msg_host_t), and they exchange data by sending tasks (#msg_task_t)
 *  that are similar to envelops containing data.
 */

/******************************** Process ************************************/
/**
 * \brief Cleans the MSG data of an actor
 * \param smx_actor a SIMIX actor
 */
void MSG_process_cleanup_from_SIMIX(smx_actor_t smx_actor)
{
  simgrid::msg::ActorExt* msg_actor;

  // get the MSG process from the SIMIX process
  if (smx_actor == SIMIX_process_self()) {
    /* avoid a SIMIX request if this function is called by the process itself */
    msg_actor = (simgrid::msg::ActorExt*)SIMIX_process_self_get_data();
    SIMIX_process_self_set_data(nullptr);
  } else {
    msg_actor = (simgrid::msg::ActorExt*)smx_actor->userdata;
    simcall_process_set_data(smx_actor, nullptr);
  }

  TRACE_msg_process_destroy(smx_actor->name, smx_actor->pid);
  // free the data if a function was provided
  if (msg_actor && msg_actor->data && msg_global->process_data_cleanup) {
    msg_global->process_data_cleanup(msg_actor->data);
  }

  delete msg_actor;
  SIMIX_process_cleanup(smx_actor);
}

/* This function creates a MSG process. It has the prototype enforced by SIMIX_function_register_process_create */
smx_actor_t MSG_process_create_from_SIMIX(const char* name, std::function<void()> code, void* data, sg_host_t host,
                                          std::map<std::string, std::string>* properties,
                                          smx_actor_t /*parent_process*/)
{
  msg_process_t p = MSG_process_create_from_stdfunc(name, std::move(code), data, host, properties);
  return p == nullptr ? nullptr : p->getImpl();
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
  return MSG_process_create_with_environment(name, code, data, host, 0, nullptr, nullptr);
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information and can be nullptr.
 * \param code is a function describing the behavior of the process. It should then only use functions described
 * in \ref m_process_management (to create a new #msg_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in \ref m_task_management (to create or destroy some #msg_task_t for example) and
   in \ref msg_task_usage (to handle file transfers and task processing).
 * \param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be nullptr. It can be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new process is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \see msg_process_t
 * \return The new corresponding object.
 */

msg_process_t MSG_process_create_with_arguments(const char *name, xbt_main_func_t code, void *data, msg_host_t host,
                                              int argc, char **argv)
{
  return MSG_process_create_with_environment(name, code, data, host, argc, argv, nullptr);
}

/** \ingroup m_process_management
 * \brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * \param name a name for the object. It is for user-level information and can be nullptr.
 * \param code is a function describing the behavior of the process. It should then only use functions described
 * in \ref m_process_management (to create a new #msg_process_t for example),
   in \ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in \ref m_task_management (to create or destroy some #msg_task_t for example) and
   in \ref msg_task_usage (to handle file transfers and task processing).
 * \param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be nullptr. It can be retrieved with the function \ref MSG_process_get_data.
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
  std::function<void()> function;
  if (code)
    function = simgrid::xbt::wrapMain(code, argc, static_cast<const char* const*>(argv));

  std::map<std::string, std::string> props;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* value;
  xbt_dict_foreach (properties, cursor, key, value)
    props[key] = value;
  xbt_dict_free(&properties);

  msg_process_t res = MSG_process_create_from_stdfunc(name, std::move(function), data, host, &props);
  for (int i = 0; i != argc; ++i)
    xbt_free(argv[i]);
  xbt_free(argv);
  return res;
}
}

msg_process_t MSG_process_create_from_stdfunc(const char* name, std::function<void()> code, void* data, msg_host_t host,
                                              std::map<std::string, std::string>* properties)
{
  xbt_assert(code != nullptr && host != nullptr, "Invalid parameters: host and code params must not be nullptr");
  simgrid::msg::ActorExt* msgExt = new simgrid::msg::ActorExt(data);

  smx_actor_t process = simcall_process_create(name, std::move(code), msgExt, host, properties);

  if (not process) { /* Undo everything */
    delete msgExt;
    return nullptr;
  }

  simcall_process_on_exit(process, (int_f_pvoid_pvoid_t)TRACE_msg_process_kill, process);
  return process->ciface();
}

extern "C" {

/* Become a process in the simulation
 *
 * Currently this can only be called by the main thread (once) and only work with some thread factories
 * (currently ThreadContextFactory).
 *
 * In the future, it might be extended in order to attach other threads created by a third party library.
 */
msg_process_t MSG_process_attach(const char *name, void *data, msg_host_t host, xbt_dict_t properties)
{
  xbt_assert(host != nullptr, "Invalid parameters: host and code params must not be nullptr");
  std::map<std::string, std::string> props;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* value;
  xbt_dict_foreach (properties, cursor, key, value)
    props[key] = value;
  xbt_dict_free(&properties);

  /* Let's create the process: SIMIX may decide to start it right now, even before returning the flow control to us */
  smx_actor_t process = SIMIX_process_attach(name, new simgrid::msg::ActorExt(data), host->getCname(), &props, nullptr);
  if (not process)
    xbt_die("Could not attach");
  simcall_process_on_exit(process,(int_f_pvoid_pvoid_t)TRACE_msg_process_kill,process);
  return process->ciface();
}

/** Detach a process attached with `MSG_process_attach()`
 *
 *  This is called when the current process has finished its job.
 *  Used in the main thread, it waits for the simulation to finish before  returning. When it returns, the other
 *  simulated processes and the maestro are destroyed.
 */
void MSG_process_detach()
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
  process->kill();
}

/**
* \brief Wait for the completion of a #msg_process_t.
*
* \param process the process to wait for
* \param timeout wait until the process is over, or the timeout occurs
*/
msg_error_t MSG_process_join(msg_process_t process, double timeout){
  simcall_process_join(process->getImpl(), timeout);
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
  TRACE_msg_process_change_host(process, host);
  process->migrate(host);
  return MSG_OK;
}

/** Yield the current actor; let the other actors execute first */
void MSG_process_yield()
{
  simgrid::simix::kernelImmediate([] { /* do nothing*/ });
}

/** \ingroup m_process_management
 * \brief Returns the user data of a process.
 *
 * This function checks whether \a process is a valid pointer and returns the user data associated to this process.
 */
void* MSG_process_get_data(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");

  /* get from SIMIX the MSG process data, and then the user data */
  simgrid::msg::ActorExt* msgExt = (simgrid::msg::ActorExt*)process->getImpl()->userdata;
  if (msgExt)
    return msgExt->data;
  else
    return nullptr;
}

/** \ingroup m_process_management
 * \brief Sets the user data of a process.
 *
 * This function checks whether \a process is a valid pointer and sets the user data associated to this process.
 */
msg_error_t MSG_process_set_data(msg_process_t process, void *data)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");

  static_cast<simgrid::msg::ActorExt*>(process->getImpl()->userdata)->data = data;

  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Sets a cleanup function to be called to free the userdata of a process when a process is destroyed.
 * \param data_cleanup a cleanup function for the userdata of a process, or nullptr to call no function
 */
XBT_PUBLIC(void) MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup) {
  msg_global->process_data_cleanup = data_cleanup;
}

/** \ingroup m_process_management
 * \brief Return the location on which a process is running.
 * \param process a process (nullptr means the current one)
 * \return the msg_host_t corresponding to the location on which \a process is running.
 */
msg_host_t MSG_process_get_host(msg_process_t process)
{
  if (process == nullptr) {
    return SIMIX_process_self()->host;
  } else {
    return process->getImpl()->host;
  }
}

/** \ingroup m_process_management
 *
 * \brief Return a #msg_process_t given its PID.
 *
 * This function search in the list of all the created msg_process_t for a msg_process_t  whose PID is equal to \a PID.
 * If no host is found, \c nullptr is returned.
   Note that the PID are uniq in the whole simulation, not only on a given host.
 */
msg_process_t MSG_process_from_PID(int PID)
{
  return SIMIX_process_from_PID(PID)->ciface();
}

/** @brief returns a list of all currently existing processes */
xbt_dynar_t MSG_processes_as_dynar() {
  xbt_dynar_t res = xbt_dynar_new(sizeof(smx_actor_t), nullptr);
  for (auto const& kv : simix_global->process_list) {
    smx_actor_t actor = kv.second;
    xbt_dynar_push(res, &actor);
  }
  return res;
}

/** @brief Return the current number MSG processes. */
int MSG_process_get_number()
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
  simcall_process_set_kill_time(process->getImpl(), kill_time);
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
  if (process == nullptr || process->getImpl() == nullptr)
    return 0;
  return process->getImpl()->pid;
}

/** \ingroup m_process_management
 * \brief Returns the process ID of the parent of \a process.
 *
 * This function checks whether \a process is a valid pointer and return its PID.
 * Returns -1 if the process has not been created by any other process.
 */
int MSG_process_get_PPID(msg_process_t process)
{
  return process->getImpl()->ppid;
}

/** \ingroup m_process_management
 * \brief Return the name of a process.
 *
 * This function checks whether \a process is a valid pointer and return its name.
 */
const char *MSG_process_get_name(msg_process_t process)
{
  return process->getCname();
}

/** \ingroup m_process_management
 * \brief Returns the value of a given process property
 *
 * \param process a process
 * \param name a property name
 * \return value of a property (or nullptr if the property is not set)
 */
const char *MSG_process_get_property_value(msg_process_t process, const char *name)
{
  return process->getProperty(name);
}

/** \ingroup m_process_management
 * \brief Return the list of properties
 *
 * This function returns all the parameters associated with a process
 */
xbt_dict_t MSG_process_get_properties(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: First argument must not be nullptr");
  xbt_dict_t as_dict = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props =
      simgrid::simix::kernelImmediate([process] { return process->getImpl()->getProperties(); });
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup m_process_management
 * \brief Return the PID of the current process.
 *
 * This function returns the PID of the currently running #msg_process_t.
 */
int MSG_process_self_PID()
{
  smx_actor_t self = SIMIX_process_self();
  return self == nullptr ? 0 : self->pid;
}

/** \ingroup m_process_management
 * \brief Return the PPID of the current process.
 *
 * This function returns the PID of the parent of the currently running #msg_process_t.
 */
int MSG_process_self_PPID()
{
  return MSG_process_get_PPID(MSG_process_self());
}

/** \ingroup m_process_management
 * \brief Return the name of the current process.
 */
const char* MSG_process_self_name()
{
  return SIMIX_process_self_get_name();
}

/** \ingroup m_process_management
 * \brief Return the current process.
 *
 * This function returns the currently running #msg_process_t.
 */
msg_process_t MSG_process_self()
{
  return SIMIX_process_self()->ciface();
}

/** \ingroup m_process_management
 * \brief Suspend the process.
 *
 * This function suspends the process by suspending the task on which it was waiting for the completion.
 */
msg_error_t MSG_process_suspend(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: First argument must not be nullptr");

  TRACE_msg_process_suspend(process);
  simcall_process_suspend(process->getImpl());
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Resume a suspended process.
 *
 * This function resumes a suspended process by resuming the task on which it was waiting for the completion.
 */
msg_error_t MSG_process_resume(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: First argument must not be nullptr");

  TRACE_msg_process_resume(process);
  process->resume();
  return MSG_OK;
}

/** \ingroup m_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 */
int MSG_process_is_suspended(msg_process_t process)
{
  return process->isSuspended();
}

smx_context_t MSG_process_get_smx_ctx(msg_process_t process) {
  return process->getImpl()->context;
}
/**
 * \ingroup m_process_management
 * \brief Add a function to the list of "on_exit" functions for the current process.
 * The on_exit functions are the functions executed when your process is killed.
 * You should use them to free the data used by your process.
 */
void MSG_process_on_exit(int_f_pvoid_pvoid_t fun, void *data) {
  simcall_process_on_exit(SIMIX_process_self(), fun, data);
}
/**
 * \ingroup m_process_management
 * \brief Sets the "auto-restart" flag of the process.
 * If the flag is set to 1, the process will be automatically restarted when its host comes back up.
 */
XBT_PUBLIC(void) MSG_process_auto_restart_set(msg_process_t process, int auto_restart) {
  process->setAutoRestart(auto_restart);
}
/**
 * \ingroup m_process_management
 * \brief Restarts a process from the beginning.
 */
XBT_PUBLIC(msg_process_t) MSG_process_restart(msg_process_t process) {
  return process->restart();
}

/** @ingroup m_process_management
 * @brief This process will be terminated automatically when the last non-daemon process finishes
 */
XBT_PUBLIC(void) MSG_process_daemonize(msg_process_t process)
{
  process->daemonize();
}

/** @ingroup m_process_management
 * @brief Take an extra reference on that process to prevent it to be garbage-collected
 */
XBT_PUBLIC(void) MSG_process_ref(msg_process_t process)
{
  intrusive_ptr_add_ref(process);
}
/** @ingroup m_process_management
 * @brief Release a reference on that process so that it can get be garbage-collected
 */
XBT_PUBLIC(void) MSG_process_unref(msg_process_t process)
{
  intrusive_ptr_release(process);
}
}
