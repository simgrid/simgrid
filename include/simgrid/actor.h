/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ACTOR_H_
#define INCLUDE_SIMGRID_ACTOR_H_

#include <simgrid/forward.h>
#include <xbt/base.h>
#include <xbt/dict.h>

/* C interface */
SG_BEGIN_DECL
/** @brief Actor datatype.
    @ingroup m_actor_management

    An actor may be defined as a <em>code</em>, with some <em>private data</em>, executing in a <em>location</em>.

    You should not access directly to the fields of the pointed structure, but always use the provided API to interact
    with actors.
 */
XBT_PUBLIC size_t sg_actor_count();
XBT_PUBLIC sg_actor_t* sg_actor_list();

/** Create and start an actor running the given code, with the given arguments, on the given host */
XBT_PUBLIC sg_actor_t sg_actor_create_(const char* name, sg_host_t host, xbt_main_func_t code, int argc,
                                       const char* const* argv);
static inline sg_actor_t sg_actor_create(const char* name, sg_host_t host, xbt_main_func_t code, int argc,
                                         char* const* argv)
{
  return sg_actor_create_(name, host, code, argc, (const char* const*)argv);
}
/** Create an actor without starting it yet. Call sg_actor_start() to actually launch it. */
XBT_PUBLIC sg_actor_t sg_actor_init(const char* name, sg_host_t host);
XBT_PUBLIC void sg_actor_start_(sg_actor_t actor, xbt_main_func_t code, int argc, const char* const* argv);
/** Start the previously initialized actor.
 *
 * Note that argv is copied over, so you should free your own copy once the actor is started. */
static inline void sg_actor_start(sg_actor_t actor, xbt_main_func_t code, int argc, char* const* argv)
{
  sg_actor_start_(actor, code, argc, (const char* const*)argv);
}
/** Start the previously initialized actor using a pthread-like API. */
void sg_actor_start_voidp(sg_actor_t actor, void_f_pvoid_t code, void* param);

XBT_PUBLIC void sg_actor_set_stacksize(sg_actor_t actor, unsigned size);

/** Kill the current actor. Only call this on the current thread. */
XBT_ATTRIB_NORETURN XBT_PUBLIC void sg_actor_exit();
/** Add a function to the list of "on_exit" functions of the current actor.
 *
 * The on_exit functions are the functions executed when your actor is killed. You should use them to free the
 * data used by your actor. */
XBT_PUBLIC void sg_actor_on_exit(void_f_int_pvoid_t fun, void* data);

/** Retrieves the actor ID (PID) of this actor */
XBT_PUBLIC aid_t sg_actor_get_pid(const_sg_actor_t actor);
/** Retrieves the actor ID (PID) of this actor's creator */
XBT_PUBLIC aid_t sg_actor_get_ppid(const_sg_actor_t actor);
/** Retrieves the actor that has the given PID (or dies if not existing) */
XBT_PUBLIC sg_actor_t sg_actor_by_pid(aid_t pid);
/** Retrieves the name of this actor */
XBT_PUBLIC const char* sg_actor_get_name(const_sg_actor_t actor);
/** Retrieves the host on which this actor is running */
XBT_PUBLIC sg_host_t sg_actor_get_host(const_sg_actor_t actor);
/** Retrieve the value of an actor property (or nullptr if not set) */
XBT_PUBLIC const char* sg_actor_get_property_value(const_sg_actor_t actor, const char* name);

#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use sg_actor_get_property_names instead: we want to kill xbt_dict at some point")
    XBT_PUBLIC xbt_dict_t sg_actor_get_properties(const_sg_actor_t actor);
#endif
/** @brief Returns a NULL-terminated list of the existing properties' names.
 *
 * if @c size is not null, the properties count is also stored in it
 * Only free the vector after use, do not mess with the names stored in it as they are the original strings, not copies.
 */
XBT_PUBLIC const char** sg_actor_get_property_names(const_sg_actor_t host, int* size);

/** Suspend this actor, that is blocked until resumed by another actor. */
XBT_PUBLIC void sg_actor_suspend(sg_actor_t actor);
/** Resume this actor that was previously suspended. */
XBT_PUBLIC void sg_actor_resume(sg_actor_t actor);
/** Returns true if this actor is currently suspended. */
XBT_PUBLIC int sg_actor_is_suspended(const_sg_actor_t actor);
/** Kill this actor and restart it from start. */
XBT_PUBLIC sg_actor_t sg_actor_restart(sg_actor_t actor);
/** Specify whether this actor shall restart when its host reboots. */
XBT_PUBLIC void sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart);
/** This actor will be automatically terminated when the last non-daemon actor finishes. */
XBT_PUBLIC void sg_actor_daemonize(sg_actor_t actor);
/** Returns whether or not this actor has been daemonized */
XBT_PUBLIC int sg_actor_is_daemon(const_sg_actor_t actor);

/** Moves this actor to another host.
 *
 * If the actor is currently blocked on an execution activity, the activity is also migrated to the new host. If
 * it's blocked on another kind of activity, an error is raised as the mandated code is not written yet. Please
 * report that bug if you need it. */
XBT_PUBLIC void sg_actor_set_host(sg_actor_t actor, sg_host_t host);
/** Wait for this actor to finish, or for the timeout to elapse. */
XBT_PUBLIC void sg_actor_join(const_sg_actor_t actor, double timeout);
/** Ask this actor to die. Any blocking activity will be canceled, and it will be rescheduled to free its memory. */
XBT_PUBLIC void sg_actor_kill(sg_actor_t actor);
/** Kill all actors (but the caller). */
XBT_PUBLIC void sg_actor_kill_all();
/** Sets the time at which this actor should be killed */
XBT_PUBLIC void sg_actor_set_kill_time(sg_actor_t actor, double kill_time);
/** Yield the current actor, give the control to the other actors. Only call this on the current thread. */
XBT_PUBLIC void sg_actor_yield();
/** Block the current actor sleeping for that amount of seconds. Only call this on the current thread. */
XBT_PUBLIC void sg_actor_sleep_for(double duration);
/** Block the current actor sleeping until the specified timestamp. Only call this on the current thread. */
XBT_PUBLIC void sg_actor_sleep_until(double wakeup_time);
XBT_PUBLIC sg_actor_t sg_actor_attach_pthread(const char* name, void* data, sg_host_t host);
#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use sg_actor_attach_pthread() instead") XBT_PUBLIC sg_actor_t
    sg_actor_attach(const char* name, void* data, sg_host_t host, xbt_dict_t properties);
#endif
XBT_PUBLIC void sg_actor_detach();
/** Retrieve a reference to the current actor */
XBT_PUBLIC sg_actor_t sg_actor_self();
/** Returns the actor ID (PID) of the current actor. */
XBT_PUBLIC aid_t sg_actor_self_get_pid();
/** Returns the actor ID (PID) of the current actor's creator. */
XBT_PUBLIC aid_t sg_actor_self_get_ppid();
/** Returns the name of the current actor (or "maestro" if maestro is running) */
XBT_PUBLIC const char* sg_actor_self_get_name();
/** Retrieve the user data associated to the current actor (or nullptr if not set) */
XBT_PUBLIC void* sg_actor_self_get_data();
/** Set the user data associated to the current actor */
XBT_PUBLIC void sg_actor_self_set_data(void* data);
/** Block the current actor, computing the given amount of flops. Only call this on the current thread. */
XBT_PUBLIC void sg_actor_execute(double flops);
/** Block the current actor, computing the given amount of flops at the given priority. Only call this on the
 * current thread. */
XBT_PUBLIC void sg_actor_execute_with_priority(double flops, double priority);
/** Block the current actor until the built parallel execution completes. */
void sg_actor_parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount);
/** Increase the reference counter of this actor */
XBT_PUBLIC void sg_actor_ref(const_sg_actor_t actor);
/** Decrease the reference counter of this actor */
XBT_PUBLIC void sg_actor_unref(const_sg_actor_t actor);
/** Retrieve the user data associated to this actor (or nullptr if not set) */
XBT_PUBLIC void* sg_actor_get_data(const_sg_actor_t actor);
/** Set the user data associated to this actor */
XBT_PUBLIC void sg_actor_set_data(sg_actor_t actor, void* userdata);

/** Initialize a sequential execution that must then be started manually. Only call this on the current thread. */
XBT_PUBLIC sg_exec_t sg_actor_exec_init(double computation_amount);
/** Initialize a parallel execution that must then be started manually. Only call this on the current thread. */
XBT_PUBLIC sg_exec_t sg_actor_parallel_exec_init(int host_nb, const sg_host_t* host_list, double* flops_amount,
                                                 double* bytes_amount);
/** Initialize and start a sequential execution. Only call this on the current thread. */
XBT_PUBLIC sg_exec_t sg_actor_exec_async(double computation_amount);
SG_END_DECL

#endif /* INCLUDE_SIMGRID_ACTOR_H_ */
