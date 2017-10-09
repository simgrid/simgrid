/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include "simgrid/s4u.hpp"
 #include <cstdlib>
 #include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/* Executed on process termination*/
static int my_onexit(void* ignored1, void *ignored2) {
  XBT_INFO("Exiting now (done sleeping or got killed)."); /* - Just display an informative message (see tesh file) */
  return 0;
}

/* Just sleep until termination */
class sleeper {

public:
  explicit sleeper(std::vector<std::string> args)
{
  XBT_INFO("Hello! I go to sleep.");
  simcall_process_on_exit(SIMIX_process_self(), my_onexit, NULL);

  simgrid::s4u::this_actor::sleep_for(std::stoi(args[1]));

}
void operator()()
{
  XBT_INFO("Done sleeping.");
}
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  e.loadPlatform(argv[1]);            /* - Load the platform description */
  e.registerFunction<sleeper>("sleeper");
  e.loadDeployment(argv[2]);   /* - Deploy the sleeper processes with explicit start/kill times */

  e.run();                           /* - Run the simulation */

  XBT_INFO("Simulation time %g", SIMIX_get_clock());
  return 0;
}
