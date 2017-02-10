/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_NETWORK_PRIVATE_H
#define _SIMIX_NETWORK_PRIVATE_H

#include "simgrid/s4u/Mailbox.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/MailboxImpl.hpp"

XBT_PRIVATE smx_activity_t SIMIX_comm_irecv(smx_actor_t dst_proc, smx_mailbox_t mbox,
                              void *dst_buff, size_t *dst_buff_size,
                              int (*match_fun)(void *, void *, smx_activity_t),
                              void (*copy_data_fun)(smx_activity_t, void*, size_t),
                              void *data, double rate);
XBT_PRIVATE smx_activity_t SIMIX_comm_iprobe(smx_actor_t dst_proc, smx_mailbox_t mbox, int type, int src,
                              int tag, int (*match_fun)(void *, void *, smx_activity_t), void *data);

#endif
