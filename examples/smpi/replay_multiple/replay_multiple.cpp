/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include "simgrid/engine.h"
#include "simgrid/s4u/Actor.hpp"
#include "xbt/replay.hpp"
#include "xbt/str.h"

#include <stdio.h>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static void smpi_replay(int argc, char* argv[])
{
  const char* instance_id = argv[1];
  int rank                = (int)xbt_str_parse_int(argv[2], "Cannot parse rank '%s'");
  const char* shared_trace =
      simgrid::s4u::Actor::self()->get_property("tracefile"); // Cannot use properties because this can be nullptr
  const char* private_trace = argv[3];
  double start_delay_flops  = 0;

  if (argc > 4) {
    start_delay_flops = xbt_str_parse_double(argv[4], "Cannot parse start_delay_flops");
  }

  if (shared_trace != nullptr)
    xbt_replay_set_tracefile(shared_trace);
  smpi_replay_run(instance_id, rank, start_delay_flops, private_trace);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  SMPI_init();

  xbt_assert(argc > 3,
             "Usage: %s description_file platform_file deployment_file\n"
             "\tExample: %s smpi_multiple_apps msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  /*  Simulation setting */
  simgrid_load_platform(argv[2]);

  /*   Application deployment: read the description file in order to identify instances to launch */
  FILE* fp = fopen(argv[1], "r");
  xbt_assert(fp != NULL, "Cannot open %s", argv[1]);

  char line[2048];
  while (fgets(line, sizeof line, fp)) {
    xbt_assert(1 + strlen(line) < sizeof line, "input buffer too short (read: %s)", line);
    xbt_dynar_t elems = xbt_str_split_quoted_in_place(line);
    xbt_assert(xbt_dynar_length(elems) >= 3, "Not enough elements in the line");

    const char** line_char  = static_cast<const char**>(xbt_dynar_to_array(elems));
    const char* instance_id = line_char[0];
    int instance_size       = (int)xbt_str_parse_int(line_char[2], "Invalid size: %s");

    XBT_INFO("Initializing instance %s of size %d", instance_id, instance_size);
    SMPI_app_instance_register(instance_id, smpi_replay, instance_size);

    xbt_free(line_char);
  }

  fclose(fp);

  simgrid_load_deployment(argv[3]);
  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  SMPI_finalize();
  return 0;
}
