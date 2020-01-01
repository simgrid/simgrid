/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "common.h"

int process_pending_connections(xbt_dynar_t q)
{
  unsigned int iter;
  int status;
  int empty = 0;
  msg_comm_t comm;

  xbt_dynar_foreach (q, iter, comm) {
    empty = 1;
    if (MSG_comm_test(comm)) {
      status = MSG_comm_get_status(comm);
      MSG_comm_destroy(comm);
      xbt_assert(status == MSG_OK, "process_pending_connections() failed");
      xbt_dynar_cursor_rm(q, &iter);
      empty = 0;
    }
  }
  return empty;
}
