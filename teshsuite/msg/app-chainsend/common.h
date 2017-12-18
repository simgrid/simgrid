/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef COMMON_H
#define COMMON_H

#include "simgrid/msg.h"
#include "xbt/sysdep.h"

static inline void queue_pending_connection(msg_comm_t comm, xbt_dynar_t q)
{
  xbt_dynar_push(q, &comm);
}

int process_pending_connections(xbt_dynar_t q);

#endif /* COMMON_H */
