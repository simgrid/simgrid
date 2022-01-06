/* Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "app-bittorrent.h"
#include "bittorrent-peer.h"
#include "tracker.h"

#include <simgrid/engine.h>
#include <xbt/asserts.h>

/** Bittorrent example launcher */
int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file", argv[0]);

  simgrid_load_platform(argv[1]);

  simgrid_register_function("tracker", tracker);
  simgrid_register_function("peer", peer);

  simgrid_load_deployment(argv[2]);

  simgrid_run();

  return 0;
}
