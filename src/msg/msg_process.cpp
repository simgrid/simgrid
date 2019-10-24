/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_process, msg, "Logging specific to MSG (process)");

std::string instr_pid(simgrid::s4u::Actor const& proc)
{
  return std::string(proc.get_name()) + "-" + std::to_string(proc.get_pid());
}

void MSG_process_userdata_init()
{
  if (not msg_global)
    msg_global = new MSG_Global_t();

  if (not simgrid::msg::ActorUserData::EXTENSION_ID.valid())
    simgrid::msg::ActorUserData::EXTENSION_ID = simgrid::s4u::Actor::extension_create<simgrid::msg::ActorUserData>();
  simgrid::s4u::Actor::on_creation.connect([](simgrid::s4u::Actor& actor) {
    XBT_DEBUG("creating the extension to store user data");
    actor.extension_set(new simgrid::msg::ActorUserData());
  });

  simgrid::s4u::Actor::on_destruction.connect([](simgrid::s4u::Actor const& actor) {
    // free the data if a function was provided
    auto extension = actor.extension<simgrid::msg::ActorUserData>();
    void* userdata = extension ? extension->get_user_data() : nullptr;
    if (userdata && msg_global->process_data_cleanup) {
      msg_global->process_data_cleanup(userdata);
    }
  });
}

/******************************** Process ************************************/
/** @brief Creates and runs a new #msg_process_t.
 *
 * Does exactly the same as #MSG_process_create_with_arguments but without providing standard arguments
 * (@a argc, @a argv, @a start_time, @a kill_time).
 */
msg_process_t MSG_process_create(const char *name, xbt_main_func_t code, void *data, msg_host_t host)
{
  return MSG_process_create_with_environment(name == nullptr ? "" : name, code, data, host, 0, nullptr, nullptr);
}

/** @brief Creates and runs a new process.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param code is a function describing the behavior of the process.
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

/**
 * @brief Creates and runs a new #msg_process_t.

 * A constructor for #msg_process_t taking four arguments and returning the corresponding object. The structure (and
 * the corresponding thread) is created, and put in the list of ready process.
 * @param name a name for the object. It is for user-level information and can be nullptr.
 * @param code is a function describing the behavior of the process.
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
  xbt_assert(host != nullptr, "Invalid parameters: host param must not be nullptr");
  simgrid::simix::ActorCode function;
  if (code)
    function = simgrid::xbt::wrap_main(code, argc, static_cast<const char* const*>(argv));

  simgrid::s4u::ActorPtr actor;

  try {
    if (data != nullptr) {
      actor = simgrid::s4u::Actor::init(std::move(name), host);
      actor->extension<simgrid::msg::ActorUserData>()->set_user_data(data);
      xbt_dict_cursor_t cursor = nullptr;
      char* key;
      char* value;
      xbt_dict_foreach (properties, cursor, key, value)
        actor->set_property(key, value);
      actor->start(std::move(function));
    } else
      actor = simgrid::s4u::Actor::create(std::move(name), host, std::move(function));
  } catch (simgrid::HostFailureException const&) {
    xbt_die("Could not launch a new process on failed host %s.", host->get_cname());
  }

  xbt_dict_free(&properties);
  for (int i = 0; i != argc; ++i)
    xbt_free(argv[i]);
  xbt_free(argv);

  simgrid::s4u::this_actor::yield();
  return actor.get();
}

/** @brief Returns the user data of a process.
 *
 * This function checks whether @a process is a valid pointer and returns the user data associated to this process.
 */
void* MSG_process_get_data(msg_process_t process)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");

  /* get from SIMIX the MSG process data, and then the user data */
  return process->extension<simgrid::msg::ActorUserData>()->get_user_data();
}

/** @brief Sets the user data of a process.
 *
 * This function checks whether @a process is a valid pointer and sets the user data associated to this process.
 */
msg_error_t MSG_process_set_data(msg_process_t process, void *data)
{
  xbt_assert(process != nullptr, "Invalid parameter: first parameter must not be nullptr!");
  process->extension<simgrid::msg::ActorUserData>()->set_user_data(data);

  return MSG_OK;
}

/** @brief Sets a cleanup function to be called to free the userdata of a process when a process is destroyed.
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

/** @brief Add a function to the list of "on_exit" functions for the current process.
 *  The on_exit functions are the functions executed when your process is killed.
 *  You should use them to free the data used by your process.
 */
void MSG_process_on_exit(int_f_int_pvoid_t fun, void* data)
{
  simgrid::s4u::this_actor::on_exit([fun, data](bool failed) { fun(failed ? 1 /*FAILURE*/ : 0 /*SUCCESS*/, data); });
}
