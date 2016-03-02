/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "mpi.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int smpi_replay(int argc, char *argv[]) {
  smpi_replay_run(&argc, &argv);
  return 0;
}

int main(int argc, char *argv[]){
  msg_error_t res;

  MSG_init(&argc, argv);

  xbt_assert(argc > 3, "Usage: %s description_file platform_file deployment_file\n"
             "\tExample: %s smpi_multiple_apps msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  /*  Simulation setting */
  MSG_create_environment(argv[2]);

  /*   Application deployment: read the description file in order to identify instances to launch */
  FILE* fp = fopen(argv[1], "r");
  if (fp == NULL)
    xbt_die("Cannot open %s", argv[1]);
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

  MSG_launch_application(argv[3]);
  SMPI_init();

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  SMPI_finalize();
  return res != MSG_OK;
}
