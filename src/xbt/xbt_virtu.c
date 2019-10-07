/* virtualization layer for XBT */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/simix.h"
#include "xbt/virtu.h"

int xbt_getpid()
{
  smx_actor_t self = SIMIX_process_self();
  return self == NULL ? 0 : sg_actor_self_get_pid();
}

const char *xbt_procname(void)
{
  return SIMIX_process_self_get_name();
}
