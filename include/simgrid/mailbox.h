/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_MAILBOX_H_
#define INCLUDE_SIMGRID_MAILBOX_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC sg_mailbox_t sg_mailbox_by_name(const char* alias);
XBT_PUBLIC const char* sg_mailbox_get_name(const_sg_mailbox_t mailbox);
XBT_PUBLIC void sg_mailbox_set_receiver(const char* alias);
XBT_PUBLIC int sg_mailbox_listen(const char* alias);

XBT_PUBLIC void sg_mailbox_put(sg_mailbox_t mailbox, void* payload, long simulated_size_in_bytes);
XBT_PUBLIC sg_comm_t sg_mailbox_put_async(sg_mailbox_t mailbox, void* payload, long simulated_size_in_bytes);
XBT_PUBLIC void* sg_mailbox_get(sg_mailbox_t mailbox);
XBT_PUBLIC sg_comm_t sg_mailbox_get_async(sg_mailbox_t mailbox, void** data);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_MAILBOX_H_ */
