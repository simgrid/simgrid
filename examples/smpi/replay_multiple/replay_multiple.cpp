/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include "simgrid/engine.h"
#include "simgrid/s4u/Actor.hpp"
#include "xbt/replay.hpp"
#include "xbt/str.h"

#include <fstream>
#include <sstream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(replay_multiple, "Messages specific for this example");

static void smpi_replay(int argc, char* argv[])
{
  const char* instance_id = argv[1];
  int rank                = static_cast<int>(xbt_str_parse_int(argv[2], "Cannot parse rank"));
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
             "\tExample: %s smpi_multiple_apps platform.xml deployment.xml\n",
             argv[0], argv[0]);

  /*  Simulation setting */
  simgrid_load_platform(argv[2]);

  /*   Application deployment: read the description file in order to identify instances to launch */
  std::ifstream f(argv[1]);
  xbt_assert(f.is_open(), "Cannot open file '%s'", argv[1]);

  std::string line;
  while (std::getline(f, line)) {
    std::string instance_id;
    std::string filename;
    int instance_size;

    std::istringstream is(line);
    is >> instance_id >> filename >> instance_size;
    xbt_assert(is, "Not enough elements in the line");

    XBT_INFO("Initializing instance %s of size %d", instance_id.c_str(), instance_size);
    SMPI_app_instance_register(instance_id.c_str(), smpi_replay, instance_size);
  }

  f.close();

  simgrid_load_deployment(argv[3]);
  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  SMPI_finalize();
  return 0;
}
