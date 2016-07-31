/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/Synchro.h"

simgrid::kernel::activity::Synchro::Synchro()
{
}

simgrid::kernel::activity::Synchro::~Synchro()
{
}

void simgrid::kernel::activity::Synchro::ref()
{
  refcount++;
}

void simgrid::kernel::activity::Synchro::unref()
{
  xbt_assert(refcount > 0,
      "This synchro has a negative refcount! You can only call test() or wait() once per synchronization.");

  refcount--;
  if (refcount>0)
    return;
  delete this;
}
