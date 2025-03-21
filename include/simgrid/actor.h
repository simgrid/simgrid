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

XBT_PUBLIC sg_actor_t sg_actor_create_(const char* name, sg_host_t host, xbt_main_func_t code, int argc,
                                       const char* const* argv);
static inline sg_actor_t sg_actor_create(const char* name, sg_host_t host, xbt_main_func_t code, int argc,
                                         char* const* argv)
{
  return sg_actor_create_(name, host, code, argc, (const char* const*)argv);
}
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

XBT_ATTRIB_NORETURN XBT_PUBLIC void sg_actor_exit();
XBT_PUBLIC void sg_actor_on_exit(void_f_int_pvoid_t fun, void* data);

XBT_PUBLIC aid_t sg_actor_get_pid(const_sg_actor_t actor);
XBT_PUBLIC aid_t sg_actor_get_ppid(const_sg_actor_t actor);
XBT_PUBLIC sg_actor_t sg_actor_by_pid(aid_t pid);
XBT_PUBLIC const char* sg_actor_get_name(const_sg_actor_t actor);
XBT_PUBLIC sg_host_t sg_actor_get_host(const_sg_actor_t actor);
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

XBT_PUBLIC void sg_actor_suspend(sg_actor_t actor);
XBT_PUBLIC void sg_actor_resume(sg_actor_t actor);
XBT_PUBLIC int sg_actor_is_suspended(const_sg_actor_t actor);
XBT_PUBLIC sg_actor_t sg_actor_restart(sg_actor_t actor);
XBT_PUBLIC void sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart);
XBT_PUBLIC void sg_actor_daemonize(sg_actor_t actor);
XBT_PUBLIC int sg_actor_is_daemon(const_sg_actor_t actor);

XBT_PUBLIC void sg_actor_set_host(sg_actor_t actor, sg_host_t host);
XBT_PUBLIC void sg_actor_join(const_sg_actor_t actor, double timeout);
XBT_PUBLIC void sg_actor_kill(sg_actor_t actor);
XBT_PUBLIC void sg_actor_kill_all();
XBT_PUBLIC void sg_actor_set_kill_time(sg_actor_t actor, double kill_time);
XBT_PUBLIC void sg_actor_yield();
XBT_PUBLIC void sg_actor_sleep_for(double duration);
XBT_PUBLIC void sg_actor_sleep_until(double wakeup_time);
XBT_PUBLIC sg_actor_t sg_actor_attach_pthread(const char* name, void* data, sg_host_t host);
#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v403("Please use sg_actor_attach_pthread() instead") XBT_PUBLIC sg_actor_t
    sg_actor_attach(const char* name, void* data, sg_host_t host, xbt_dict_t properties);
#endif
XBT_PUBLIC void sg_actor_detach();
XBT_PUBLIC sg_actor_t sg_actor_self();
XBT_PUBLIC aid_t sg_actor_self_get_pid();
XBT_PUBLIC aid_t sg_actor_self_get_ppid();
/** Returns the name of the current actor (or "maestro" if maestro is running) */
XBT_PUBLIC const char* sg_actor_self_get_name();
XBT_PUBLIC void* sg_actor_self_get_data();
XBT_PUBLIC void sg_actor_self_set_data(void* data);
XBT_PUBLIC void sg_actor_execute(double flops);
XBT_PUBLIC void sg_actor_execute_with_priority(double flops, double priority);
void sg_actor_parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount);
XBT_PUBLIC void sg_actor_ref(const_sg_actor_t actor);
XBT_PUBLIC void sg_actor_unref(const_sg_actor_t actor);
XBT_PUBLIC void* sg_actor_get_data(const_sg_actor_t actor);
XBT_PUBLIC void sg_actor_set_data(sg_actor_t actor, void* userdata);

XBT_PUBLIC sg_exec_t sg_actor_exec_init(double computation_amount);
XBT_PUBLIC sg_exec_t sg_actor_parallel_exec_init(int host_nb, const sg_host_t* host_list, double* flops_amount,
                                                 double* bytes_amount);
XBT_PUBLIC sg_exec_t sg_actor_exec_async(double computation_amount);
SG_END_DECL

#endif /* INCLUDE_SIMGRID_ACTOR_H_ */
