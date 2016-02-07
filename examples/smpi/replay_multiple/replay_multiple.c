/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */
#include "mpi.h"
/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "smpi/smpi.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
    "Messages specific for this msg example");

static int smpi_replay(int argc, char *argv[]) {
  smpi_replay_run(&argc, &argv);
  return 0;
}

int main(int argc, char *argv[]){
  msg_error_t res;
  const char *platform_file;
  const char *application_file;
  const char *description_file;

  MSG_init(&argc, argv);

  if (argc < 4) {
    printf("Usage: %s description_file platform_file deployment_file\n", argv[0]);
    printf("example: %s smpi_multiple_apps msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  description_file = argv[1];
  platform_file = argv[2];
  application_file = argv[3];


  /*  Simulation setting */
  MSG_create_environment(platform_file);

  /*   Application deployment: read the description file in order to identify instances to launch */
  FILE* fp = fopen(description_file, "r");
  if (fp == NULL)
    xbt_die("Cannot open %s", description_file);
  ssize_t read;
  char   *line = NULL;
  size_t n   = 0;
  int instance_size = 0;
  const char* instance_id = NULL;
  while ((read = xbt_getline(&line, &n, fp)) != -1 ){
    xbt_dynar_t elems = xbt_str_split_quoted_in_place(line);
    if(xbt_dynar_length(elems)<3){
      xbt_die ("Not enough elements in the line");
    }

    const char** line_char= xbt_dynar_to_array(elems);
    instance_id = line_char[0];
    instance_size = xbt_str_parse_int(line_char[2], "Invalid size: %s");

    XBT_INFO("Initializing instance %s of size %d", instance_id, instance_size);
    SMPI_app_instance_register(instance_id, smpi_replay,instance_size);

    xbt_free(line_char);
  }

  MSG_launch_application(application_file);
  SMPI_init();

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  SMPI_finalize();
  if (res == MSG_OK)
    return 0;
  else
    return 1;

}
