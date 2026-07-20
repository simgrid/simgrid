/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_MAILBOX_H_
#define INCLUDE_SIMGRID_MAILBOX_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL

/** @brief Retrieve the mailbox associated to the given name. Mailboxes are created on demand. */
XBT_PUBLIC sg_mailbox_t sg_mailbox_by_name(const char* alias);
/** @brief Retrieves the name of this mailbox */
XBT_PUBLIC const char* sg_mailbox_get_name(const_sg_mailbox_t mailbox);
/** @brief Declare that the actor of the given name is a permanent receiver on the mailbox of the given name.
 *
 * It means that the communications sent to this mailbox will start flowing to its host even before it does a
 * get(). This models the real behavior of TCP and MPI communications, amongst other.
 */
XBT_PUBLIC void sg_mailbox_set_receiver(const char* alias);
/** @brief Check if there is a communication going on in the mailbox of the given name. */
XBT_PUBLIC int sg_mailbox_listen(const char* alias);

/** @brief Creates (but don't start) a data transmission to this mailbox */
XBT_PUBLIC sg_comm_t sg_mailbox_put_init(sg_mailbox_t mailbox, void* payload, long simulated_size_in_bytes);
/** @brief Creates and start a data transmission to this mailbox */
XBT_PUBLIC sg_comm_t sg_mailbox_put_async(sg_mailbox_t mailbox, void* payload, long simulated_size_in_bytes);
/** @brief Blocking data transmission */
XBT_PUBLIC void sg_mailbox_put(sg_mailbox_t mailbox, void* payload, long simulated_size_in_bytes);

/** @brief Non-blocking data reception */
XBT_PUBLIC sg_comm_t sg_mailbox_get_async(sg_mailbox_t mailbox, void** data);
/** @brief Blocking data reception */
XBT_PUBLIC void* sg_mailbox_get(sg_mailbox_t mailbox);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_MAILBOX_H_ */
