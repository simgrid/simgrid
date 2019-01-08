/* virtualization layer for XBT */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt/virtu.h"

int xbt_getpid()
{
  return MSG_process_self_PID();
}

const char *xbt_procname(void)
{
  return MSG_process_self_name();
}
