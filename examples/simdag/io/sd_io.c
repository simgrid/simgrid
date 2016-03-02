/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_io, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  unsigned int ctr;
  xbt_dict_t current_storage_list;
  char *mount_name;
  char *storage_name;
  xbt_dict_cursor_t cursor = NULL;

  SD_init(&argc, argv);
  /* Set the workstation model to default, as storage is not supported by the ptask_L07 model yet. */
  SD_config("host/model", "default");
  SD_create_environment(argv[1]);
  const sg_host_t *workstations = sg_host_list();
  int total_nworkstations = sg_host_count();

  for (ctr=0; ctr<total_nworkstations;ctr++){
    current_storage_list = sg_host_get_mounted_storage_list(workstations[ctr]);
    xbt_dict_foreach(current_storage_list,cursor,mount_name,storage_name)
      XBT_INFO("Workstation '%s' mounts '%s'", sg_host_get_name(workstations[ctr]), mount_name);
    xbt_dict_free(&current_storage_list);
  }

  SD_exit();
  return 0;
}
