/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "bittorrent.h"
#include "peer.h"
#include "tracker.h"
#include <msg/msg.h>
#include <xbt/RngStream.h>

/**
 * Bittorrent example launcher
 */
int main(int argc, char *argv[])
{
  xbt_dynar_t host_list;
  msg_host_t host;
  unsigned i;

  MSG_init(&argc, argv);

  /* Check the arguments */
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file \n", argv[0]);
    return -1;
  }

  const char *platform_file = argv[1];
  const char *deployment_file = argv[2];

  MSG_create_environment(platform_file);

  host_list = MSG_hosts_as_dynar();
  xbt_dynar_foreach(host_list, i, host) {
    char descr[512];
    RngStream stream;
    snprintf(descr, sizeof descr, "RngSream<%s>", MSG_host_get_name(host));
    stream = RngStream_CreateStream(descr);
    MSG_host_set_data(host, stream);
  }

  MSG_function_register("tracker", tracker);
  MSG_function_register("peer", peer);

  MSG_launch_application(deployment_file);

  MSG_main();

  xbt_dynar_foreach(host_list, i, host) {
    RngStream stream = MSG_host_get_data(host);
    RngStream_DeleteStream(&stream);
  }
  xbt_dynar_free(&host_list);

  return 0;
}
