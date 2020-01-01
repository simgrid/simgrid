/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage, "Messages specific for this simulation");

static int host(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char* host_name = MSG_host_get_name(MSG_host_self());

  /* - Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on %s ***", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;

  /* - Retrieve all mount points of current host */
  xbt_dict_t storage_list = MSG_host_get_mounted_storage_list(MSG_host_self());

  /* - For each disk mounted on host, display disk name and mount point */
  xbt_dict_foreach (storage_list, cursor, mount_name, storage_name)
    XBT_INFO("Storage name: %s, mount name: %s", storage_name, mount_name);

  xbt_dict_free(&storage_list);

  /* - Write 200,000 bytes on Disk4 */
  msg_storage_t storage = MSG_storage_get_by_name("Disk4");
  sg_size_t write       = MSG_storage_write(storage, 200000); // Write 200,000 bytes
  XBT_INFO("Wrote %llu bytes on 'Disk4'", write);

  /*  - Now read 200,000 bytes */
  sg_size_t read = MSG_storage_read(storage, 200000);
  XBT_INFO("Read %llu bytes on 'Disk4'", read);

  /* - Attach some user data to disk1 */
  XBT_INFO("*** Get/set data for storage element: Disk4 ***");

  char* data = MSG_storage_get_data(storage);

  XBT_INFO("Get storage data: '%s'", data);

  MSG_storage_set_data(storage, xbt_strdup("Some user data"));
  data = MSG_storage_get_data(storage);
  XBT_INFO("Set and get data: '%s'", data);
  xbt_free(data);

  return 1;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);

  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  MSG_process_create(NULL, host, NULL, xbt_dynar_get_as(hosts, 3, msg_host_t));
  xbt_dynar_free(&hosts);

  msg_error_t res = MSG_main();
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  return res != MSG_OK;
}
