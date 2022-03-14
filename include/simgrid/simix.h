/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_H
#define SIMGRID_SIMIX_H

#include <simgrid/forward.h>
#include <xbt/parmap.h>

#ifndef SIMIX_H_NO_DEPRECATED_WARNING
#warning simgrid/simix.h is deprecated and will be removed in v3.35.
#endif

/******************************* Networking ***********************************/
SG_BEGIN_DECL

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
XBT_ATTRIB_DEPRECATED_v333("Please use kernel::actor::ActorImpl::self()") XBT_PUBLIC
#ifdef __cplusplus
    simgrid::kernel::actor::ActorImpl*
#else
    smx_actor_t
#endif
    SIMIX_process_self();
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

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

/************************** Communication simcalls ****************************/

XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::send()") XBT_PUBLIC
    void simcall_comm_send(simgrid::kernel::actor::ActorImpl* sender, simgrid::kernel::activity::MailboxImpl* mbox,
                           double task_size, double rate, void* src_buff, size_t src_buff_size,
                           bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                           double timeout);

XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::isend()") XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
    simcall_comm_isend(simgrid::kernel::actor::ActorImpl* sender, simgrid::kernel::activity::MailboxImpl* mbox,
                       double task_size, double rate, void* src_buff, size_t src_buff_size,
                       bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*), void (*clean_fun)(void*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       bool detached);

XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::recv()") XBT_PUBLIC
    void simcall_comm_recv(simgrid::kernel::actor::ActorImpl* receiver, simgrid::kernel::activity::MailboxImpl* mbox,
                           void* dst_buff, size_t* dst_buff_size,
                           bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                           void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                           double timeout, double rate);

XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::irecv()") XBT_PUBLIC simgrid::kernel::activity::ActivityImplPtr
    simcall_comm_irecv(simgrid::kernel::actor::ActorImpl* receiver, simgrid::kernel::activity::MailboxImpl* mbox,
                       void* dst_buff, size_t* dst_buff_size,
                       bool (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(simgrid::kernel::activity::CommImpl*, void*, size_t), void* data,
                       double rate);

XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::wait_any_for()") XBT_PUBLIC ssize_t
    simcall_comm_waitany(simgrid::kernel::activity::CommImpl* comms[], size_t count, double timeout);
XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::wait_for()") XBT_PUBLIC
    void simcall_comm_wait(simgrid::kernel::activity::ActivityImpl* comm, double timeout);
XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::test()") XBT_PUBLIC
    bool simcall_comm_test(simgrid::kernel::activity::ActivityImpl* comm);
XBT_ATTRIB_DEPRECATED_v335("Please use s4u::Comm::test_any()") XBT_PUBLIC ssize_t
    simcall_comm_testany(simgrid::kernel::activity::CommImpl* comms[], size_t count);

#endif
#endif
