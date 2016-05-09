/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroExec.hpp"
#include "src/surf/surf_interface.hpp"

simgrid::simix::Exec::~Exec()
{
  if (surf_exec)
    surf_exec->unref();
}
void simgrid::simix::Exec::suspend()
{
  if (surf_exec)
    surf_exec->suspend();
}

void simgrid::simix::Exec::resume()
{
  if (surf_exec)
    surf_exec->resume();
}

double simgrid::simix::Exec::remains()
{
  if (state == SIMIX_RUNNING)
    return surf_exec->getRemains();

  return 0;
}
