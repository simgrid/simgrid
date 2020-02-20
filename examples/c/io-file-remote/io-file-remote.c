/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/disk.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include <simgrid/plugins/file_system.h>

#include "xbt/asserts.h"
#include "xbt/log.h"

#define INMEGA (1024 * 1024)

XBT_LOG_NEW_DEFAULT_CATEGORY(remote_io, "Messages specific for this io example");

static void host(int argc, char* argv[])
{
  sg_file_t file       = sg_file_open(argv[1], NULL);
  const char* filename = sg_file_get_name(file);
  XBT_INFO("Opened file '%s'", filename);
  sg_file_dump(file);

  XBT_INFO("Try to write %llu MiB to '%s'", sg_file_get_size(file) / 1024, filename);
  sg_size_t write = sg_file_write(file, sg_file_get_size(file) * 1024);
  XBT_INFO("Have written %llu MiB to '%s'.", write / (1024 * 1024), filename);

  if (argc > 4) {
    if (atoi(argv[4]) != 0) {
      XBT_INFO("Move '%s' (of size %llu) from '%s' to '%s'", filename, sg_file_get_size(file), sg_host_self_get_name(),
               argv[2]);
      sg_file_rmove(file, sg_host_by_name(argv[2]), argv[3]);
    } else {
      XBT_INFO("Copy '%s' (of size %llu) from '%s' to '%s'", filename, sg_file_get_size(file), sg_host_self_get_name(),
               argv[2]);
      sg_file_rcopy(file, sg_host_by_name(argv[2]), argv[3]);
    }
  }
  sg_file_close(file);
}

int main(int argc, char** argv)
{

  simgrid_init(&argc, argv);
  sg_storage_file_system_init();
  simgrid_load_platform(argv[1]);

  simgrid_register_function("host", host);
  simgrid_load_deployment(argv[2]);

  size_t host_count = sg_host_count();
  sg_host_t* hosts  = sg_host_list();

  for (long i = 0; i < host_count; i++) {
    unsigned int disk_count;
    sg_disk_t* disks;
    sg_host_disks(hosts[i], &disk_count, &disks);
    for (unsigned int j = 0; j < disk_count; j++)
      XBT_INFO("Init: %s: %llu/%llu MiB used/free on '%s@%s'", sg_host_get_name(hosts[i]),
               sg_disk_get_size_used(disks[j]) / INMEGA, sg_disk_get_size_free(disks[j]) / INMEGA,
               sg_disk_name(disks[j]), sg_host_get_name(sg_disk_get_host(disks[j])));
    free(disks);
  }

  simgrid_run();

  for (long i = 0; i < host_count; i++) {
    unsigned int disk_count;
    sg_disk_t* disks;
    sg_host_disks(hosts[i], &disk_count, &disks);
    for (unsigned int j = 0; j < disk_count; j++)
      XBT_INFO("End: %llu/%llu MiB used/free on '%s@%s'", sg_disk_get_size_used(disks[j]) / INMEGA,
               sg_disk_get_size_free(disks[j]) / INMEGA, sg_disk_name(disks[j]), sg_host_get_name(hosts[i]));
    free(disks);
  }

  free(hosts);

  XBT_INFO("Simulation time %g", simgrid_get_clock());
  return 0;
}
