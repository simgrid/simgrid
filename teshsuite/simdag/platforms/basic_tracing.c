/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/msg.h"

int host(int argc, char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(basic_tracing,"Messages specific for this example");

int host(int argc, char *argv[])
{
  XBT_INFO("Sleep for 1s");
  MSG_process_sleep(1);
  return 0;
}

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  xbt_dynar_t all_hosts = MSG_hosts_as_dynar();
  MSG_process_create( "host", host, NULL, xbt_dynar_pop_as(all_hosts,msg_host_t));
  xbt_dynar_free(&all_hosts);

  int res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
