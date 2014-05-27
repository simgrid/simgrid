/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * @subsection MSG_ex_resources Other resource kinds
 * 
 * This section contains some sparse examples of how to use the other
 * kind of resources, such as disk or GPU. These resources are quite
 * experimental for now, but here we go anyway.
 * 
 * - <b>io/remote.c</b> Example of delegated I/O operations
 */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "surf/surf_private.h"

int host(int argc, char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(remote_io,
                             "Messages specific for this io example");


int host(int argc, char *argv[]){
  msg_file_t file = NULL;
  const char* filename;
  sg_size_t read, write;

  file = MSG_file_open(argv[1], NULL);
  filename = MSG_file_get_name(file);
  XBT_INFO("Opened file '%s'",filename);
  MSG_file_dump(file);

  XBT_INFO("Try to read %llu from '%s'",MSG_file_get_size(file),filename);
  read = MSG_file_read(file, MSG_file_get_size(file));
  XBT_INFO("Have read %llu from '%s'. Offset is now at: %llu",read,filename,
      MSG_file_tell(file));
  XBT_INFO("Seek back to the begining of the stream...");
  MSG_file_seek(file, 0, SEEK_SET);
  XBT_INFO("Offset is now at: %llu", MSG_file_tell(file));

  MSG_file_close(file);

  if (argc > 2){
    file = MSG_file_open(argv[2], NULL);
    filename = MSG_file_get_name(file);
    XBT_INFO("Opened file '%s'",filename);
    XBT_INFO("Try to write 100KiB to '%s'",filename);
    write = MSG_file_write(file, 100*1024);
    XBT_INFO("Have written %llu bytes to '%s'. Offset is now at: %llu",write,filename,
        MSG_file_tell(file));

    msg_host_t src, dest;
    src= MSG_host_self();
    dest = MSG_get_host_by_name(argv[3]);
    XBT_INFO("Move '%s' (of size %llu) from '%s' to '%s'", filename,
           MSG_file_get_size(file), MSG_host_get_name(src),
           argv[3]);
    MSG_file_rmove(file, dest, argv[4]);
  }

  return 0;
}



int main(int argc, char **argv)
{
  int res;

  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
