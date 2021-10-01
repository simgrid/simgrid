/* virtualization layer for XBT */

/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/virtu.h"

int xbt_getpid()
{
  const auto* self = simgrid::kernel::actor::ActorImpl::self();
  return self == nullptr ? 0 : static_cast<int>(self->get_pid());
}

const char* xbt_procname(void)
{
  return SIMIX_is_maestro() ? "maestro" : simgrid::kernel::actor::ActorImpl::self()->get_cname();
}
