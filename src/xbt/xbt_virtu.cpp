/* virtualization layer for XBT */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.h"
#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/virtu.h"

int xbt_getpid()
{
  smx_actor_t self = SIMIX_process_self();
  return self == nullptr ? 0 : self->get_pid();
}

const char* xbt_procname(void)
{
  return SIMIX_process_self_get_name();
}
