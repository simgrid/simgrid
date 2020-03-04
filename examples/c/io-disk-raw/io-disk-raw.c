/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/disk.h"
#include "simgrid/engine.h"
#include "simgrid/forward.h"
#include "simgrid/host.h"
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <stddef.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(disk, "Messages specific for this simulation");

static void host(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  const char* host_name = sg_host_get_name(sg_host_self());

  /* - Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on %s ***", host_name);

  /* - Retrieve all disks from current host */
  unsigned int disk_count;
  sg_disk_t* disk_list;
  sg_host_disks(sg_host_self(), &disk_count, &disk_list);

  for (unsigned int i = 0; i < disk_count; i++)
    XBT_INFO("Disk name: %s (read: %.0f B/s -- write: %.0f B/s ", sg_disk_name(disk_list[i]),
             sg_disk_read_bandwidth(disk_list[i]), sg_disk_write_bandwidth(disk_list[i]));

  /* - Write 400,000 bytes on Disk1 */
  sg_disk_t disk  = disk_list[0];
  sg_size_t write = sg_disk_write(disk, 400000);
  XBT_INFO("Wrote %llu bytes on '%s'", write, sg_disk_name(disk));

  /*  - Now read 200,000 bytes */
  sg_size_t read = sg_disk_read(disk, 200000);
  XBT_INFO("Read %llu bytes on '%s'", read, sg_disk_name(disk));

  /* - Attach some user data to disk1 */
  XBT_INFO("*** Get/set data for storage element: Disk1 ***");

  char* data = (char*)sg_disk_data(disk);

  XBT_INFO("Get storage data: '%s'", data ? data : "No user data");

  sg_disk_data_set(disk, xbt_strdup("Some user data"));
  data = (char*)sg_disk_data(disk);
  XBT_INFO("Set and get data: '%s'", data);
  free(data);
  free(disk_list);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);

  simgrid_register_function("host", host);

  size_t host_count = sg_host_count();
  sg_host_t* hosts  = sg_host_list();

  for (long i = 0; i < host_count; i++) {
    XBT_INFO("*** %s properties ****", sg_host_get_name(hosts[i]));
    xbt_dict_t props         = sg_host_get_properties(hosts[i]);
    xbt_dict_cursor_t cursor = NULL;
    char* key;
    void* data;
    xbt_dict_foreach (props, cursor, key, data)
      XBT_INFO("  %s -> %s", key, (char*)data);
    xbt_dict_free(&props);
  }

  free(hosts);

  sg_actor_create("", sg_host_by_name("bob"), host, 0, NULL);

  simgrid_run();

  XBT_INFO("Simulated time %g", simgrid_get_clock());

  return 0;
}
