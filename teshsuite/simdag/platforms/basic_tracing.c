/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "surf/surf_private.h"

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
  int res;
  xbt_dynar_t all_hosts;
  m_host_t first_host;
  MSG_global_init(&argc, argv);
  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  all_hosts = MSG_hosts_as_dynar();
  first_host = xbt_dynar_pop_as(all_hosts,m_host_t);
  MSG_process_create( "host", host, NULL, first_host);
  xbt_dynar_free(&all_hosts);

  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  MSG_clean();
  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
