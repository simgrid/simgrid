/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_H
#define SIMGRID_SIMIX_H

#include <simgrid/forward.h>

// avoid deprecation warning on include (remove entire file with XBT_ATTRIB_DEPRECATED_v335)
#ifndef SIMIX_H_NO_DEPRECATED_WARNING
#warning simgrid/simix.h is deprecated and will be removed in v3.35.
#endif

#ifdef __cplusplus

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
