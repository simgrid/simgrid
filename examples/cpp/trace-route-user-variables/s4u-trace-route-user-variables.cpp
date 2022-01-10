/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void trace_fun()
{
  auto bourassa = simgrid::s4u::Host::by_name("Bourassa");
  auto fafard   = simgrid::s4u::Host::by_name("Fafard");
  auto ginette  = simgrid::s4u::Host::by_name("Ginette");
  auto tremblay = simgrid::s4u::Host::by_name("Tremblay");

  // Set initial values for the link user variables
  // This example uses source and destination where source and destination are the name of hosts in the platform file.
  // The functions will set/change the value of the variable for all links in the route between source and destination.

  // Set the Link_Capacity variable
  simgrid::instr::set_link_variable(tremblay, bourassa, "Link_Capacity", 12.34);
  simgrid::instr::set_link_variable(fafard, ginette, "Link_Capacity", 56.78);

  // Set the Link_Utilization variable
  simgrid::instr::set_link_variable(tremblay, bourassa, "Link_Utilization", 1.2);
  simgrid::instr::set_link_variable(fafard, ginette, "Link_Utilization", 3.4);

  // run the simulation, update my variables accordingly
  for (int i = 0; i < 10; i++) {
    simgrid::s4u::this_actor::execute(1e6);

    // Add to link user variables
    simgrid::instr::add_link_variable(tremblay, bourassa, "Link_Utilization", 5.6);
    simgrid::instr::add_link_variable(fafard, ginette, "Link_Utilization", 7.8);
  }

  for (int i = 0; i < 10; i++) {
    simgrid::s4u::this_actor::execute(1e6);

    // Subtract from link user variables
    simgrid::instr::sub_link_variable(tremblay, bourassa, "Link_Utilization", 3.4);
    simgrid::instr::sub_link_variable(fafard, ginette, "Link_Utilization", 5.6);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring link user variables (one without, another with an RGB color)
  simgrid::instr::declare_link_variable("Link_Capacity");
  simgrid::instr::declare_link_variable("Link_Utilization", "0.9 0.1 0.1");

  simgrid::s4u::Actor::create("master", e.host_by_name("Tremblay"), trace_fun);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Tremblay"), trace_fun);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Jupiter"), trace_fun);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Fafard"), trace_fun);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Ginette"), trace_fun);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Bourassa"), trace_fun);

  e.run();
  return 0;
}
