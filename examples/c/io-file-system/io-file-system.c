/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/disk.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/plugins/file_system.h"

#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <stdio.h> /* SEEK_SET */

XBT_LOG_NEW_DEFAULT_CATEGORY(io_file_system, "Messages specific for this io example");

static void show_info(unsigned int disk_count, const sg_disk_t* disks)
{
  XBT_INFO("Storage info on %s:", sg_host_self_get_name());

  for (unsigned int i = 0; i < disk_count; i++) {
    const_sg_disk_t d = disks[i];
    // Retrieve disk's information
    XBT_INFO("    %s (%s) Used: %llu; Free: %llu; Total: %llu.", sg_disk_name(d), sg_disk_get_mount_point(d),
             sg_disk_get_size_used(d), sg_disk_get_size_free(d), sg_disk_get_size(d));
  }
}

static void host(int argc, char* argv[])
{
  unsigned int disk_count;
  sg_disk_t* disks;
  sg_host_disks(sg_host_self(), &disk_count, &disks);

  show_info(disk_count, disks);

  // Open an non-existing file to create it
  const char* filename = "/scratch/tmp/data.txt";
  sg_file_t file       = sg_file_open(filename, NULL);

  sg_size_t write = sg_file_write(file, 200000); // Write 200,000 bytes
  XBT_INFO("Create a %llu bytes file named '%s' on /scratch", write, filename);

  // check that sizes have changed
  show_info(disk_count, disks);

  // Now retrieve the size of created file and read it completely
  const sg_size_t file_size = sg_file_get_size(file);
  sg_file_seek(file, 0, SEEK_SET);
  const sg_size_t read = sg_file_read(file, file_size);
  XBT_INFO("Read %llu bytes on %s", read, filename);

  // Now write 100,000 bytes in tmp/data.txt
  write = sg_file_write(file, 100000); // Write 100,000 bytes
  XBT_INFO("Write %llu bytes on %s", write, filename);

  // Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
  const char* newpath = "/scratch/tmp/simgrid.readme";
  XBT_INFO("Move '%s' to '%s'", sg_file_get_name(file), newpath);
  sg_file_move(file, newpath);

  // Test attaching some user data to the file
  sg_file_set_data(file, xbt_strdup("777"));
  char* file_data = (char*)(sg_file_get_data(file));
  XBT_INFO("User data attached to the file: %s", file_data);
  xbt_free(file_data);

  // Close the file
  sg_file_close(file);

  show_info(disk_count, disks);

  // Reopen the file and then unlink it
  file = sg_file_open("/scratch/tmp/simgrid.readme", NULL);
  XBT_INFO("Unlink file: '%s'", sg_file_get_name(file));
  sg_file_unlink(file);

  show_info(disk_count, disks);
  xbt_free(disks);
}

int main(int argc, char** argv)
{
  simgrid_init(&argc, argv);
  sg_storage_file_system_init();
  simgrid_load_platform(argv[1]);
  sg_actor_create("host", sg_host_by_name("bob"), host, 0, NULL);
  simgrid_run();

  return 0;
}
