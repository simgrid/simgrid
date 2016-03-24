/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>io/remote.c</b> Example of delegated I/O operations
 */

#include "simgrid/msg.h"

#define INMEGA (1024*1024)

XBT_LOG_NEW_DEFAULT_CATEGORY(remote_io, "Messages specific for this io example");

static int host(int argc, char *argv[]){
  msg_file_t file = MSG_file_open(argv[1], NULL);
  const char *filename = MSG_file_get_name(file);
  XBT_INFO("Opened file '%s'",filename);
  MSG_file_dump(file);

  XBT_INFO("Try to read %llu from '%s'",MSG_file_get_size(file),filename);
  sg_size_t read = MSG_file_read(file, MSG_file_get_size(file));
  XBT_INFO("Have read %llu from '%s'. Offset is now at: %llu",read,filename, MSG_file_tell(file));
  XBT_INFO("Seek back to the begining of the stream...");
  MSG_file_seek(file, 0, SEEK_SET);
  XBT_INFO("Offset is now at: %llu", MSG_file_tell(file));

  MSG_file_close(file);

  if (argc > 5){
    file = MSG_file_open(argv[2], NULL);
    filename = MSG_file_get_name(file);
    XBT_INFO("Opened file '%s'",filename);
    XBT_INFO("Try to write %llu MiB to '%s'", MSG_file_get_size(file)/1024, filename);
    sg_size_t write = MSG_file_write(file, MSG_file_get_size(file)*1024);
    XBT_INFO("Have written %llu bytes to '%s'.",write,filename);

    msg_host_t src, dest;
    src= MSG_host_self();
    dest = MSG_host_by_name(argv[3]);
    if (xbt_str_parse_int(argv[5], "Argument 5 (move or copy) must be an int, not '%s'")) {
      XBT_INFO("Move '%s' (of size %llu) from '%s' to '%s'", filename,MSG_file_get_size(file), MSG_host_get_name(src),
               argv[3]);
      MSG_file_rmove(file, dest, argv[4]);
    } else {
      XBT_INFO("Copy '%s' (of size %llu) from '%s' to '%s'", filename, MSG_file_get_size(file), MSG_host_get_name(src),
               argv[3]);
      MSG_file_rcopy(file, dest, argv[4]);
      MSG_file_close(file);
    }
  }

  return 0;
}

int main(int argc, char **argv)
{
  unsigned int cur;
  msg_storage_t st;

  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  MSG_launch_application(argv[2]);

  xbt_dynar_t storages = MSG_storages_as_dynar();
  xbt_dynar_foreach(storages, cur, st){
    XBT_INFO("Init: %llu MiB used on '%s'", MSG_storage_get_used_size(st)/INMEGA, MSG_storage_get_name(st));
  }

  int res = MSG_main();

  xbt_dynar_foreach(storages, cur, st){
    XBT_INFO("Init: %llu MiB used on '%s'", MSG_storage_get_used_size(st)/INMEGA, MSG_storage_get_name(st));
  }
  xbt_dynar_free_container(&storages);

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
