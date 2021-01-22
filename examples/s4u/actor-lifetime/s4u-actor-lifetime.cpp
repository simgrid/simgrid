/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This C++ file acts as the foil to the corresponding XML file, where the
   action takes place: Actors are started and stopped at predefined time.   */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Messages specific for this s4u example");

/* This actor just sleeps until termination */
class sleeper {
public:
  explicit sleeper(std::vector<std::string> /*args*/)
  {
    sg4::this_actor::on_exit([](bool /*failed*/) {
      /* Executed on actor termination, to display a message helping to understand the output */
      XBT_INFO("Exiting now (done sleeping or got killed).");
    });
  }
  void operator()() const
  {
    XBT_INFO("Hello! I go to sleep.");
    sg4::this_actor::sleep_for(10);
    XBT_INFO("Done sleeping.");
  }
};

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s ../platforms/cluster_backbone.xml ./s4u_actor_lifetime_d.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]); /* Load the platform description */
  e.register_actor<sleeper>("sleeper");
  e.load_deployment(argv[2]); /*  Deploy the sleeper actors with explicit start/kill times */

  e.run(); /* - Run the simulation */

  return 0;
}
