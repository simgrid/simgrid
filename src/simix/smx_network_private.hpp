/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_NETWORK_PRIVATE_HPP
#define SIMIX_NETWORK_PRIVATE_HPP

#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/simix/ActorImpl.hpp"

XBT_PRIVATE smx_activity_t SIMIX_comm_irecv(smx_actor_t dst_proc, smx_mailbox_t mbox, void* dst_buff,
                                            size_t* dst_buff_size,
                                            int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                            void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data,
                                            double rate);
XBT_PRIVATE smx_activity_t SIMIX_comm_iprobe(smx_actor_t dst_proc, smx_mailbox_t mbox, int type,
                                             simix_match_func_t match_fun, void* data);

#endif
