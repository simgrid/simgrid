/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "bittorrent.h"
#include "peer.h"
#include "tracker.h"
#include <msg/msg.h>
/**
 * Bittorrent example launcher
 */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  /* Check the arguments */
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file \n", argv[0]);
    return -1;
  }

  const char *platform_file = argv[1];
  const char *deployment_file = argv[2];

  MSG_create_environment(platform_file);

  MSG_function_register("tracker", tracker);
  MSG_function_register("peer", peer);

  MSG_launch_application(deployment_file);

  MSG_main();

  MSG_clean();

  return 0;
}
