/* virtualization layer for XBT */

/* Copyright (c) 2007-2014. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt/function_types.h"
#include "xbt/misc.h"
#include "xbt/virtu.h"

static int xbt_fake_pid(void)
{
  return 0;
}

int_f_void_t xbt_getpid = &xbt_fake_pid;

const char *xbt_procname(void)
{
  return MSG_process_self_name();
}
