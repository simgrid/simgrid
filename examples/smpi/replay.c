/* Copyright (c) 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi/smpi.h"

int main(int argc, char *argv[])
{
  smpi_replay_init(&argc, &argv);

  /* Actually do the simulation using smpi_action_trace_run */
  smpi_action_trace_run(argv[1]);  // it's ok to pass a NULL argument here

  smpi_replay_finalize();
  return 0;
}
