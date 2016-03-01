/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include <unistd.h>

#define FILENAME1 "/sd1/doc/simgrid/examples/cxx/autoDestination/Main.cxx"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

static int host(int argc, char *argv[])
{
  char name[2048];
  sprintf(name,"%s%i", FILENAME1,MSG_process_self_PID());
  msg_file_t file = MSG_file_open(name, NULL);
  //MSG_file_read(file, MSG_file_get_size(file));
  MSG_file_write(file, 500000);

  XBT_INFO("Size of %s: %llu", MSG_file_get_name(file), MSG_file_get_size(file));
  MSG_file_close(file);

  return 1;
}

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);

  MSG_function_register("host", host);
  for(int i = 0 ; i<10; i++){
    MSG_process_create(xbt_strdup("host"), host, NULL, MSG_host_by_name(xbt_strdup("host")));
  }

  int res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
