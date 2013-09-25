/* Copyright (c) 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_io,
                             "Logging specific to this SimDag example");
int main(int argc, char **argv)
{
  unsigned int ctr, ctr2;
  const SD_workstation_t *workstations;
  int total_nworkstations;
  xbt_dynar_t current_storage_list;
  char *mount_name;

  SD_init(&argc, argv);
  /* Set the workstation model to default, as storage is not supported by the
   * ptask_L07 model yet.
   */
  SD_config("workstation/model", "default");
  SD_create_environment(argv[1]);
  workstations = SD_workstation_get_list();
  total_nworkstations = SD_workstation_get_number();

  for (ctr=0; ctr<total_nworkstations;ctr++){
    current_storage_list = SD_workstation_get_storage_list(workstations[ctr]);
    xbt_dynar_foreach(current_storage_list, ctr2, mount_name)
      XBT_INFO("Workstation '%s' mounts '%s'",
         SD_workstation_get_name(workstations[ctr]), mount_name);
    xbt_dynar_free_container(&current_storage_list);
  }
  SD_exit();
  return 0;
}
