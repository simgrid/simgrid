/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "bittorrent.h"
#include "bittorrent-peer.h"
#include "tracker.h"
#include <simgrid/msg.h>

/** Bittorrent example launcher */
int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file", argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("tracker", tracker);
  MSG_function_register("peer", peer);

  MSG_launch_application(argv[2]);

  MSG_main();

  return 0;
}
