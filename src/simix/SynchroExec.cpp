/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroExec.hpp"
#include "src/surf/surf_interface.hpp"

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
