/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef KADEPLOY_COMMON_H
#define KADEPLOY_COMMON_H

#include "msg/msg.h"
#include "xbt/sysdep.h"

static XBT_INLINE void queue_pending_connection(msg_comm_t comm, xbt_dynar_t q)
{
  xbt_dynar_push(q, &comm);
}

int process_pending_connections(xbt_dynar_t q);

#endif /* KADEPLOY_COMMON_H */
