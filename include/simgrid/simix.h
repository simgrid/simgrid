/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_H
#define SIMGRID_SIMIX_H

#include <simgrid/forward.h>
#include <xbt/dynar.h>
#include <xbt/ex.h>
#include <xbt/parmap.h>
#ifdef __cplusplus
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#endif

/******************************* Networking ***********************************/
SG_BEGIN_DECL

XBT_ATTRIB_DEPRECATED_v331("Please use sg_actor_by_pid() instead.") XBT_PUBLIC smx_actor_t
    SIMIX_process_from_PID(aid_t pid);

/* parallelism */
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::context::is_parallel()") XBT_PUBLIC int SIMIX_context_is_parallel();
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::context::get_nthreads()") XBT_PUBLIC int SIMIX_context_get_nthreads();
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::context::set_nthreads()") XBT_PUBLIC
    void SIMIX_context_set_nthreads(int nb_threads);
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::context::get_parallel_mode()") XBT_PUBLIC e_xbt_parmap_mode_t
    SIMIX_context_get_parallel_mode();
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::context::set_parallel_mode()") XBT_PUBLIC
    void SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode);
XBT_ATTRIB_DEPRECATED_v333("Please use Actor::is_maestro()") XBT_PUBLIC int SIMIX_is_maestro();

/********************************** Global ************************************/
/* Set some code to execute in the maestro (must be used before the engine creation)
 *
 * If no maestro code is registered (the default), the main thread
 * is assumed to be the maestro. */
XBT_ATTRIB_DEPRECATED_v333("Please use simgrid_set_maestro()") XBT_PUBLIC
    void SIMIX_set_maestro(void (*code)(void*), void* data);

/* Simulation execution */
XBT_ATTRIB_DEPRECATED_v332("Please use EngineImpl:run()") XBT_PUBLIC void SIMIX_run();
XBT_ATTRIB_DEPRECATED_v332("Please use simgrid_get_clock() or Engine::get_clock()") XBT_PUBLIC double SIMIX_get_clock();

SG_END_DECL

/********************************* Process ************************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::actor::ActorImpl::self()") XBT_PUBLIC smx_actor_t SIMIX_process_self();
XBT_ATTRIB_DEPRECATED_v333("Please use xbt_procname()") XBT_PUBLIC const char* SIMIX_process_self_get_name();
SG_END_DECL

/****************************** Communication *********************************/
#ifdef __cplusplus
XBT_ATTRIB_DEPRECATED_v333("Please use Engine::set_default_comm_data_copy_callback()") XBT_PUBLIC
    void SIMIX_comm_set_copy_data_callback(void (*callback)(simgrid::kernel::activity::CommImpl*, void*, size_t));

XBT_ATTRIB_DEPRECATED_v333("Please use Comm::copy_buffer_callback()") XBT_PUBLIC
    void SIMIX_comm_copy_pointer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size);
XBT_ATTRIB_DEPRECATED_v333("Please use Comm::copy_pointer_callback()") XBT_PUBLIC
    void SIMIX_comm_copy_buffer_callback(simgrid::kernel::activity::CommImpl* comm, void* buff, size_t buff_size);
#endif

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

/******************************* Host simcalls ********************************/
#ifdef __cplusplus
XBT_ATTRIB_DEPRECATED_v331("Please use s4u::Exec::wait_any_for()") XBT_PUBLIC
    unsigned int simcall_execution_waitany_for(simgrid::kernel::activity::ExecImpl* execs[], size_t count,
                                               double timeout);
#endif

/************************** Communication simcalls ****************************/

#ifdef __cplusplus
XBT_PUBLIC void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                                  size_t src_buff_size,
                                  bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout);

XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                   size_t src_buff_size, bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*clean_fun)(void*), void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                   void* data, bool detached);

XBT_PUBLIC void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                  bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t),
                                  void* data, double timeout, double rate);

XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                   bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                   void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data, double rate);

XBT_PUBLIC ssize_t simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count, double timeout);
XBT_PUBLIC void simcall_comm_wait(simgrid::kernel::activity::ActivityImpl* comm, double timeout);
XBT_PUBLIC bool simcall_comm_test(simgrid::kernel::activity::ActivityImpl* comm);
XBT_PUBLIC ssize_t simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count);

#endif

/************************** Synchro simcalls **********************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v331("Please use sg_mutex_lock()") XBT_PUBLIC void simcall_mutex_lock(smx_mutex_t mutex);
XBT_ATTRIB_DEPRECATED_v331("Please use sg_mutex_try_lock()") XBT_PUBLIC int simcall_mutex_trylock(smx_mutex_t mutex);
XBT_ATTRIB_DEPRECATED_v331("Please use sg_mutex_unlock()") XBT_PUBLIC void simcall_mutex_unlock(smx_mutex_t mutex);

XBT_ATTRIB_DEPRECATED_v331("Please use sg_cond_wait()") XBT_PUBLIC
    void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_ATTRIB_DEPRECATED_v331("Please use sg_cond_wait_for()") XBT_PUBLIC
    int simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double max_duration);

XBT_ATTRIB_DEPRECATED_v331("Please use sg_sem_acquire()") XBT_PUBLIC void simcall_sem_acquire(smx_sem_t sem);
XBT_ATTRIB_DEPRECATED_v331("Please use sg_sem_acquire_timeout()") XBT_PUBLIC
    int simcall_sem_acquire_timeout(smx_sem_t sem, double max_duration);
SG_END_DECL

/************************** MC simcalls   **********************************/
SG_BEGIN_DECL
XBT_ATTRIB_DEPRECATED_v331("Please use MC_random()") XBT_PUBLIC int simcall_mc_random(int min, int max);
SG_END_DECL

#endif
