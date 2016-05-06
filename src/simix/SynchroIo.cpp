/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroIo.hpp"
#include "src/surf/surf_interface.hpp"

void simgrid::simix::Io::suspend()
{
  if (surf_io)
    surf_io->suspend();
}

void simgrid::simix::Io::resume()
{
  if (surf_io)
    surf_io->resume();
}
