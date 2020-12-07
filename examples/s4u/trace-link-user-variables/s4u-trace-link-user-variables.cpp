/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

static void trace_fun()
{
  // set initial values for the link user variables this example only shows for links identified by "6" and "3" in the
  // platform file

  // Set the Link_Capacity variable
  TRACE_link_variable_set("6", "Link_Capacity", 12.34);
  TRACE_link_variable_set("3", "Link_Capacity", 56.78);

  // Set the Link_Utilization variable
  TRACE_link_variable_set("3", "Link_Utilization", 1.2);
  TRACE_link_variable_set("6", "Link_Utilization", 3.4);

  // run the simulation, update my variables accordingly
  for (int i = 0; i < 10; i++) {
    simgrid::s4u::this_actor::execute(1e6);

    // Add to link user variables
    TRACE_link_variable_add("3", "Link_Utilization", 5.6);
    TRACE_link_variable_add("6", "Link_Utilization", 7.8);
  }

  for (int i = 0; i < 10; i++) {
    simgrid::s4u::this_actor::execute(1e6);

    // Subtract from link user variables
    TRACE_link_variable_sub("3", "Link_Utilization", 3.4);
    TRACE_link_variable_sub("6", "Link_Utilization", 5.6);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring link user variables (one without, another with an RGB color)
  TRACE_link_variable_declare("Link_Capacity");
  TRACE_link_variable_declare_with_color("Link_Utilization", "0.9 0.1 0.1");

  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), trace_fun);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Tremblay"), trace_fun);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), trace_fun);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Fafard"), trace_fun);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Ginette"), trace_fun);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Bourassa"), trace_fun);

  e.run();
  return 0;
}
