/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include <unistd.h>

#define FILENAME1 "/home/doc/simgrid/examples/platforms/g5k.xml"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

static int host(int argc, char *argv[])
{
  char name[2048];
  int id = MSG_process_self_PID();
  sprintf(name,"%s%i", FILENAME1, id);
  msg_file_t file = MSG_file_open(name, NULL);
  XBT_INFO("process %d is writing!", id);
  MSG_file_write(file, 3000000);
  XBT_INFO("process %d goes to sleep for %d seconds", id, id);
  MSG_process_sleep(id);
  XBT_INFO("process %d is writing again!", id);
  MSG_file_write(file, 3000000);
  XBT_INFO("process %d goes to sleep for %d seconds", id, 6 - id);
  MSG_process_sleep(6-id);
  XBT_INFO("process %d is reading!", id);
  MSG_file_seek(file, 0, SEEK_SET);
  MSG_file_read(file, 3000000);
  XBT_INFO("process %d goes to sleep for %d seconds", id, id);
  MSG_process_sleep(id);
  XBT_INFO("process %d is reading again!", id);
  MSG_file_seek(file, 0, SEEK_SET);
  MSG_file_read(file, 3000000);

  XBT_INFO("process %d => Size of %s: %llu", id, MSG_file_get_name(file), MSG_file_get_size(file));
  MSG_file_close(file);

  return 0;
}

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);

  MSG_function_register("host", host);
  for(int i = 0 ; i < 5; i++){
    MSG_process_create("bob", host, NULL, MSG_host_by_name(xbt_strdup("bob")));
  }

  int res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
