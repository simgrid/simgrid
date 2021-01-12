/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

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

XBT_PUBLIC sg_actor_t sg_actor_create(const char* name, sg_host_t host, xbt_main_func_t code, int argc,
                                      const char* const* argv);
XBT_PUBLIC sg_actor_t sg_actor_init(const char* name, sg_host_t host);
/** Start the previously initialized actor.
 *
 * Note that argv is copied over, so you should free your own copy once the actor is started. */
XBT_PUBLIC void sg_actor_start(sg_actor_t actor, xbt_main_func_t code, int argc, const char* const* argv);
XBT_PUBLIC void sg_actor_set_stacksize(sg_actor_t actor, unsigned size);

XBT_PUBLIC void sg_actor_exit();
XBT_PUBLIC void sg_actor_on_exit(void_f_int_pvoid_t fun, void* data);

XBT_PUBLIC aid_t sg_actor_get_PID(const_sg_actor_t actor);
XBT_PUBLIC aid_t sg_actor_get_PPID(const_sg_actor_t actor);
XBT_PUBLIC sg_actor_t sg_actor_by_PID(aid_t pid);
XBT_PUBLIC const char* sg_actor_get_name(const_sg_actor_t actor);
XBT_PUBLIC sg_host_t sg_actor_get_host(const_sg_actor_t actor);
XBT_PUBLIC const char* sg_actor_get_property_value(const_sg_actor_t actor, const char* name);
XBT_PUBLIC xbt_dict_t sg_actor_get_properties(const_sg_actor_t actor);
XBT_PUBLIC void sg_actor_suspend(sg_actor_t actor);
XBT_PUBLIC void sg_actor_resume(sg_actor_t actor);
XBT_PUBLIC int sg_actor_is_suspended(const_sg_actor_t actor);
XBT_PUBLIC sg_actor_t sg_actor_restart(sg_actor_t actor);
XBT_PUBLIC void sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart);
XBT_PUBLIC void sg_actor_daemonize(sg_actor_t actor);
XBT_PUBLIC int sg_actor_is_daemon(const_sg_actor_t actor);

#ifndef DOXYGEN
XBT_ATTRIB_DEPRECATED_v329("Please use sg_actor_set_host() instead") XBT_PUBLIC
    void sg_actor_migrate(sg_actor_t process, sg_host_t host);
#endif

XBT_PUBLIC void sg_actor_set_host(sg_actor_t actor, sg_host_t host);
XBT_PUBLIC void sg_actor_join(const_sg_actor_t actor, double timeout);
XBT_PUBLIC void sg_actor_kill(sg_actor_t actor);
XBT_PUBLIC void sg_actor_kill_all();
XBT_PUBLIC void sg_actor_set_kill_time(sg_actor_t actor, double kill_time);
XBT_PUBLIC void sg_actor_yield();
XBT_PUBLIC void sg_actor_sleep_for(double duration);
XBT_PUBLIC void sg_actor_sleep_until(double wakeup_time);
XBT_PUBLIC sg_actor_t sg_actor_attach(const char* name, void* data, sg_host_t host, xbt_dict_t properties);
XBT_PUBLIC void sg_actor_detach();
XBT_PUBLIC sg_actor_t sg_actor_self();
XBT_PUBLIC aid_t sg_actor_self_get_pid();
XBT_PUBLIC aid_t sg_actor_self_get_ppid();
XBT_PUBLIC const char* sg_actor_self_get_name();
XBT_PUBLIC void* sg_actor_self_get_data();
XBT_PUBLIC void sg_actor_self_set_data(void* data);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_actor_self_get_data() instead") XBT_PUBLIC void* sg_actor_self_data();
XBT_ATTRIB_DEPRECATED_v330("Please use sg_actor_self_set_data() instead") XBT_PUBLIC
    void sg_actor_self_data_set(void* data);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_actor_execute() instead") XBT_PUBLIC void sg_actor_self_execute(double flops);
XBT_PUBLIC void sg_actor_execute(double flops);
XBT_PUBLIC void sg_actor_execute_with_priority(double flops, double priority);
void sg_actor_parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount);
XBT_PUBLIC void sg_actor_ref(const_sg_actor_t actor);
XBT_PUBLIC void sg_actor_unref(const_sg_actor_t actor);
XBT_PUBLIC void* sg_actor_get_data(const_sg_actor_t actor);
XBT_PUBLIC void sg_actor_set_data(sg_actor_t actor, void* userdata);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_actor_get_data() instead") XBT_PUBLIC
    void* sg_actor_data(const_sg_actor_t actor);
XBT_ATTRIB_DEPRECATED_v330("Please use sg_actor_set_data() instead") XBT_PUBLIC
    void sg_actor_data_set(sg_actor_t actor, void* userdata);

XBT_PUBLIC sg_exec_t sg_actor_exec_init(double computation_amount);
XBT_PUBLIC sg_exec_t sg_actor_parallel_exec_init(int host_nb, const sg_host_t* host_list, double* flops_amount,
                                                 double* bytes_amount);
XBT_PUBLIC sg_exec_t sg_actor_exec_async(double computation_amount);
SG_END_DECL

#endif /* INCLUDE_SIMGRID_ACTOR_H_ */
