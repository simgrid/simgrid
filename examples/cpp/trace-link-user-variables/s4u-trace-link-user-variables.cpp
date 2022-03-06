/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

namespace sg4 = simgrid::s4u;

static void trace_fun()
{
  // set initial values for the link user variables this example only shows for links identified by "6" and "3" in the
  // platform file
  // Set the Link_Capacity variable
  simgrid::instr::set_link_variable("6", "Link_Capacity", 12.34);
  simgrid::instr::set_link_variable("3", "Link_Capacity", 56.78);

  // Set the Link_Utilization variable
  simgrid::instr::set_link_variable("3", "Link_Utilization", 1.2);
  simgrid::instr::set_link_variable("6", "Link_Utilization", 3.4);

  // run the simulation, update my variables accordingly
  for (int i = 0; i < 10; i++) {
    sg4::this_actor::execute(1e6);

    // Add to link user variables
    simgrid::instr::add_link_variable("3", "Link_Utilization", 5.6);
    simgrid::instr::add_link_variable("6", "Link_Utilization", 7.8);
  }

  for (int i = 0; i < 10; i++) {
    sg4::this_actor::execute(1e6);

    // Subtract from link user variables
    simgrid::instr::sub_link_variable("3", "Link_Utilization", 3.4);
    simgrid::instr::sub_link_variable("6", "Link_Utilization", 5.6);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring link user variables (one without, another with an RGB color)
  simgrid::instr::declare_link_variable("Link_Capacity");
  simgrid::instr::declare_link_variable("Link_Utilization", "0.9 0.1 0.1");

  sg4::Actor::create("master", e.host_by_name("Tremblay"), trace_fun);
  sg4::Actor::create("worker", e.host_by_name("Tremblay"), trace_fun);
  sg4::Actor::create("worker", e.host_by_name("Jupiter"), trace_fun);
  sg4::Actor::create("worker", e.host_by_name("Fafard"), trace_fun);
  sg4::Actor::create("worker", e.host_by_name("Ginette"), trace_fun);
  sg4::Actor::create("worker", e.host_by_name("Bourassa"), trace_fun);

  e.run();
  return 0;
}
