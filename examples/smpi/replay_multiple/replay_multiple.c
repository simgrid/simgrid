/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "mpi.h"

#include <stdio.h>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int smpi_replay(int argc, char *argv[]) {
  const char* instance_id    = argv[1];
  int rank                   = xbt_str_parse_int(argv[2], "Cannot parse rank '%s'");
  const char* trace_filename = argv[3];
  double start_delay_flops   = 0;

  if (argc > 4) {
    start_delay_flops = xbt_str_parse_double(argv[4], "Cannot parse start_delay_flops");
  }

  smpi_replay_run(instance_id, rank, start_delay_flops, trace_filename);
  return 0;
}

int main(int argc, char *argv[]){
  msg_error_t res;

  MSG_init(&argc, argv);
  SMPI_init();

  xbt_assert(argc > 3, "Usage: %s description_file platform_file deployment_file\n"
             "\tExample: %s smpi_multiple_apps msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  /*  Simulation setting */
  MSG_create_environment(argv[2]);

  /*   Application deployment: read the description file in order to identify instances to launch */
  FILE* fp = fopen(argv[1], "r");
  if (fp == NULL)
    xbt_die("Cannot open %s", argv[1]);
  char line[2048];
  const char* instance_id = NULL;
  while (fgets(line, sizeof line, fp)) {
    xbt_assert(1 + strlen(line) < sizeof line, "input buffer too short (read: %s)", line);
    xbt_dynar_t elems = xbt_str_split_quoted_in_place(line);
    if(xbt_dynar_length(elems)<3){
      xbt_die ("Not enough elements in the line");
    }

    const char** line_char= xbt_dynar_to_array(elems);
    instance_id = line_char[0];
    int instance_size     = xbt_str_parse_int(line_char[2], "Invalid size: %s");

    XBT_INFO("Initializing instance %s of size %d", instance_id, instance_size);
    SMPI_app_instance_register(instance_id, smpi_replay,instance_size);

    xbt_free(line_char);
  }

  fclose(fp);

  MSG_launch_application(argv[3]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  SMPI_finalize();
  return res != MSG_OK;
}
