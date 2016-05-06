/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroComm.hpp"
#include "src/surf/surf_interface.hpp"

void simgrid::simix::Comm::suspend() {
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surf_comm)
    surf_comm->suspend();
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */

}

void simgrid::simix::Comm::resume() {
  /*FIXME: check what happen with the timeouts */
  if (surf_comm)
    surf_comm->resume();
  /* in the other case, the synchro were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}
