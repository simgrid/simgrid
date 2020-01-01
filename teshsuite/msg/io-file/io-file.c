/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/file_system.h"

#include <stdio.h> /* SEEK_SET */

XBT_LOG_NEW_DEFAULT_CATEGORY(io_file, "Messages specific for this io example");

static int host(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_file_t file = NULL;
  sg_size_t read;
  sg_size_t write;
  const char* st_name;

  switch (MSG_process_self_PID()) {
    case 1:
      file    = MSG_file_open("/tmp/include/surf/simgrid_dtd.h", NULL);
      st_name = "Disk2";
      break;
    case 2:
      file    = MSG_file_open("/home/doc/simgrid/examples/platforms/nancy.xml", NULL);
      st_name = "Disk1";
      break;
    case 3:
      file    = MSG_file_open("/home/doc/simgrid/examples/platforms/g5k_cabinets.xml", NULL);
      st_name = "Disk3";
      break;
    case 4:
      file = MSG_file_open("/home/doc/simgrid/examples/platforms/g5k.xml", NULL);
      MSG_file_dump(file);
      st_name = "Disk4";
      break;
    default:
      xbt_die("FILENAME NOT DEFINED %s", MSG_process_get_name(MSG_process_self()));
  }

  const char* filename = MSG_file_get_name(file);
  XBT_INFO("\tOpen file '%s'", filename);
  const_sg_storage_t st = MSG_storage_get_by_name(st_name);

  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu", filename, MSG_storage_get_used_size(st),
           MSG_storage_get_size(st));

  /* Try to read for 10MB */
  read = MSG_file_read(file, 10000000);
  XBT_INFO("\tHave read %llu from '%s'", read, filename);

  /* Write 100KB in file from the current position, i.e, end of file or 10MB */
  write = MSG_file_write(file, 100000);
  XBT_INFO("\tHave written %llu in '%s'. Size now is: %llu", write, filename, MSG_file_get_size(file));

  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu", filename, MSG_storage_get_used_size(st),
           MSG_storage_get_size(st));

  /* rewind to the beginning of the file */
  XBT_INFO("\tComing back to the beginning of the stream for file '%s'", filename);
  MSG_file_seek(file, 0, SEEK_SET);

  /* Try to read 110KB */
  read = MSG_file_read(file, 110000);
  XBT_INFO("\tHave read %llu from '%s' (of size %llu)", read, filename, MSG_file_get_size(file));

  /* rewind once again to the beginning of the file */
  XBT_INFO("\tComing back to the beginning of the stream for file '%s'", filename);
  MSG_file_seek(file, 0, SEEK_SET);

  /* Write 110KB in file from the current position, i.e, end of file or 10MB */
  write = MSG_file_write(file, 110000);
  XBT_INFO("\tHave written %llu in '%s'. Size now is: %llu", write, filename, MSG_file_get_size(file));

  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu", filename, MSG_storage_get_used_size(st),
           MSG_storage_get_size(st));

  if (MSG_process_self_PID() == 1) {
    XBT_INFO("\tUnlink file '%s'", MSG_file_get_name(file));
    MSG_file_unlink(file);
  } else {
    XBT_INFO("\tClose file '%s'", filename);
    MSG_file_close(file);
  }
  return 0;
}

int main(int argc, char** argv)
{
  MSG_init(&argc, argv);
  MSG_storage_file_system_init();

  MSG_create_environment(argv[1]);
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  MSG_function_register("host", host);
  unsigned long nb_hosts = xbt_dynar_length(hosts);
  XBT_INFO("Number of host '%lu'", nb_hosts);
  for (int i = 0; i < nb_hosts; i++) {
    MSG_process_create("host", host, NULL, xbt_dynar_get_as(hosts, i, msg_host_t));
  }
  xbt_dynar_free(&hosts);

  int res = MSG_main();
  XBT_INFO("Simulation time %.6f", MSG_get_clock());
  return res != MSG_OK;
}
