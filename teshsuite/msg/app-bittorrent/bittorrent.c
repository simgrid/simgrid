/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "bittorrent.h"
#include "bittorrent-peer.h"
#include "tracker.h"
#include <simgrid/msg.h>

#include <stdio.h> /* snprintf */

/** Bittorrent example launcher */
int main(int argc, char* argv[])
{
  msg_host_t host;
  unsigned i;

  MSG_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file", argv[0]);

  MSG_create_environment(argv[1]);

  xbt_dynar_t host_list = MSG_hosts_as_dynar();
  xbt_dynar_foreach (host_list, i, host) {
    char descr[512];
    snprintf(descr, sizeof descr, "RngSream<%s>", MSG_host_get_name(host));
  }

  MSG_function_register("tracker", tracker);
  MSG_function_register("peer", peer);

  MSG_launch_application(argv[2]);

  MSG_main();

  xbt_dynar_free(&host_list);

  return 0;
}
