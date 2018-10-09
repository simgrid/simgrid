/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_process, msg, "Logging specific to MSG (process)");

std::string instr_pid(msg_process_t proc)
{
  return std::string(proc->get_cname()) + "-" + std::to_string(proc->get_pid());
}

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
 * @brief Cleans the MSG data of an actor
 * @param smx_actor a SIMIX actor
 */
void MSG_process_cleanup_from_SIMIX(smx_actor_t smx_actor)
{
  // free the data if a function was provided
  void* userdata = smx_actor->get_user_data();
  if (userdata && msg_global->process_data_cleanup) {
    msg_global->process_data_cleanup(userdata);
  }

  SIMIX_process_cleanup(smx_actor);
}

/* This function creates a MSG process. It has the prototype enforced by SIMIX_function_register_process_create */
smx_actor_t MSG_process_create_from_SIMIX(std::string name, simgrid::simix::ActorCode code, void* data, sg_host_t host,
                                          std::unordered_map<std::string, std::string>* properties,
                                          smx_actor_t /*parent_process*/)
{
  msg_process_t p = MSG_process_create_from_stdfunc(name, std::move(code), data, host, properties);
  return p == nullptr ? nullptr : p->get_impl();
}

/** @brief Creates and runs a new #msg_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without providing standard arguments
 * (@a argc, @a argv, @a start_time, @a kill_time).
 */
msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
{
  return MSG_process_create_with_environment(name == nullptr ? "" : name, code, data, host, 0, nullptr, nullptr);
}

/** @brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param code is a function describing the behavior of the process. It should then only use functions described
 * in @ref m_process_management (to create a new #msg_process_t for example),
   in @ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in @ref m_task_management (to create or destroy some #msg_task_t for example) and
   in @ref msg_task_usage (to handle file transfers and task processing).
 * @param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be nullptr. It can be retrieved with the function @ref MSG_process_get_data.
 * @param host the location where the new process is executed.
 * @param argc first argument passed to @a code
 * @param argv second argument passed to @a code
 */

msg_process_t MSG_process_create_with_arguments(const char *name, xbt_main_func_t code, void *data, msg_host_t host,
                                              int argc, char **argv)
{
  return MSG_process_create_with_environment(name, code, data, host, argc, argv, nullptr);
}

/** @ingroup m_process_management
 * @brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param code is a function describing the behavior of the process. It should then only use functions described
 * in @ref m_process_management (to create a new #msg_process_t for example),
   in @ref m_host_management (only the read-only functions i.e. whose name contains the word get),
   in @ref m_task_management (to create or destroy some #msg_task_t for example) and
   in @ref msg_task_usage (to handle file transfers and task processing).
 * @param data a pointer to any data one may want to attach to the new object.  It is for user-level information and
 *        can be nullptr. It can be retrieved with the function @ref MSG_process_get_data.
 * @param host the location where the new process is executed.
 * @param argc first argument passed to @a code
 * @param argv second argument passed to @a code. WARNING, these strings are freed by the SimGrid kernel when the
 *             process exits, so they cannot be static nor shared between several processes.
 * @param properties list a properties defined for this process
 * @see msg_process_t
 * @return The new corresponding object.
 */
msg_process_t MSG_process_create_with_environment(const char *name, xbt_main_func_t code, void *data, msg_host_t host,
                                                  int argc, char **argv, xbt_dict_t properties)
{
  simgrid::simix::ActorCode function;
  if (code)
    function = simgrid::xbt::wrap_main(code, argc, static_cast<const char* const*>(argv));

  std::unordered_map<std::string, std::string> props;
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

msg_process_t MSG_process_create_from_stdfunc(std::string name, simgrid::simix::ActorCode code, void* data,
                                              msg_host_t host, std::unordered_map<std::string, std::string>* properties)
{
  xbt_assert(code != nullptr && host != nullptr, "Invalid parameters: host and code params must not be nullptr");

  smx_actor_t process = simcall_process_create(name, std::move(code), data, host, properties);

  if (process == nullptr)
    return nullptr;

  MSG_process_yield();
  return process->ciface();
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
  xbt_assert(host != nullptr, "Invalid parameters: host and code params must not be nullptr");
  std::unordered_map<std::string, std::string> props;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* value;
  xbt_dict_foreach (properties, cursor, key, value)
    props[key] = value;
  xbt_dict_free(&properties);

  /* Let's create the process: SIMIX may decide to start it right now, even before returning the flow control to us */
  smx_actor_t process = SIMIX_process_attach(name, data, host->get_cname(), &props, nullptr);
  if (not process)
    xbt_die("Could not attach");
  MSG_process_yield();
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

/** @ingroup m_process_management
 * @brief Returns the user data of a process.
 *
 * This function checks whether @a process is a valid pointer and returns the user data associated to this process.
 */
void* MSG_process_get_data(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");

  /* get from SIMIX the MSG process data, and then the user data */
  return process->get_impl()->get_user_data();
}

/** @ingroup m_process_management
 * @brief Sets the user data of a process.
 *
 * This function checks whether @a process is a valid pointer and sets the user data associated to this process.
 */
msg_error_t MSG_process_set_data(msg_process_t process, void *data)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");

  process->get_impl()->set_user_data(data);

  return MSG_OK;
}

/** @ingroup m_process_management
 * @brief Sets a cleanup function to be called to free the userdata of a process when a process is destroyed.
 * @param data_cleanup a cleanup function for the userdata of a process, or nullptr to call no function
 */
XBT_PUBLIC void MSG_process_set_data_cleanup(void_f_pvoid_t data_cleanup)
{
  msg_global->process_data_cleanup = data_cleanup;
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

/** @ingroup m_process_management
 * @brief Return the PID of the current process.
 *
 * This function returns the PID of the currently running #msg_process_t.
 */
int MSG_process_self_PID()
{
  smx_actor_t self = SIMIX_process_self();
  return self == nullptr ? 0 : self->pid_;
}

/** @ingroup m_process_management
 * @brief Return the PPID of the current process.
 *
 * This function returns the PID of the parent of the currently running #msg_process_t.
 */
int MSG_process_self_PPID()
{
  return MSG_process_get_PPID(MSG_process_self());
}

/** @ingroup m_process_management
 * @brief Return the name of the current process.
 */
const char* MSG_process_self_name()
{
  return SIMIX_process_self_get_name();
}

/** @ingroup m_process_management
 * @brief Return the current process.
 *
 * This function returns the currently running #msg_process_t.
 */
msg_process_t MSG_process_self()
{
  return SIMIX_process_self()->ciface();
}

smx_context_t MSG_process_get_smx_ctx(msg_process_t process) { // deprecated -- smx_context_t should die afterward
  return process->get_impl()->context_;
}
/**
 * @ingroup m_process_management
 * @brief Add a function to the list of "on_exit" functions for the current process.
 * The on_exit functions are the functions executed when your process is killed.
 * You should use them to free the data used by your process.
 */
void MSG_process_on_exit(int_f_pvoid_pvoid_t fun, void *data) {
  simgrid::s4u::this_actor::on_exit([fun](int a, void* b) { fun((void*)(intptr_t)a, b); }, data);
}

/** @ingroup m_process_management
 * @brief Take an extra reference on that process to prevent it to be garbage-collected
 */
XBT_PUBLIC void MSG_process_ref(msg_process_t process)
{
  intrusive_ptr_add_ref(process);
}
/** @ingroup m_process_management
 * @brief Release a reference on that process so that it can get be garbage-collected
 */
XBT_PUBLIC void MSG_process_unref(msg_process_t process)
{
  intrusive_ptr_release(process);
}
