/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/synchro/Synchro.h"

simgrid::simix::Synchro::Synchro()
{
}

simgrid::simix::Synchro::~Synchro()
{
}

void simgrid::simix::Synchro::ref()
{
  refcount++;
}

void simgrid::simix::Synchro::unref()
{
  xbt_assert(refcount > 0,
      "This synchro has a negative refcount! You can only call test() or wait() once per synchronization.");

  refcount--;
  if (refcount>0)
    return;
  delete this;
}
