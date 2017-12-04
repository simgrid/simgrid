/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This C++ file acts as the foil to the corresponding XML file, where the
   action takes place: Actors are started and stopped at predefined time.   */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific for this s4u example");

/* Executed on process termination, to display a message helping to understand the output */
static int my_onexit(void*, void*)
{
  XBT_INFO("Exiting now (done sleeping or got killed).");
  return 0;
}

/* Just sleep until termination */
class sleeper {

public:
  explicit sleeper(std::vector<std::string> /*args*/)
  {
    XBT_INFO("Hello! I go to sleep.");
    simgrid::s4u::this_actor::onExit(my_onexit, NULL);

    simgrid::s4u::this_actor::sleep_for(10);
  }
  void operator()() { XBT_INFO("Done sleeping."); }
};

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  e.loadPlatform(argv[1]); /* - Load the platform description */
  e.registerFunction<sleeper>("sleeper");
  e.loadDeployment(argv[2]); /* - Deploy the sleeper processes with explicit start/kill times */

  e.run(); /* - Run the simulation */

  return 0;
}
