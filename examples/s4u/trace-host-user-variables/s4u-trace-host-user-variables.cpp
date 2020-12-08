/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

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
  const char* hostname = simgrid::s4u::this_actor::get_host()->get_cname();

  // the hostname has an empty HDD with a capacity of 100000 (bytes)
  TRACE_host_variable_set(hostname, "HDD_capacity", 100000);
  TRACE_host_variable_set(hostname, "HDD_utilization", 0);

  for (int i = 0; i < 10; i++) {
    // create and execute a task just to make the simulated time advance
    simgrid::s4u::this_actor::execute(1e4);

    // ADD: after the execution of this task, the HDD utilization increases by 100 (bytes)
    TRACE_host_variable_add(hostname, "HDD_utilization", 100);
  }

  for (int i = 0; i < 10; i++) {
    // create and execute a task just to make the simulated time advance
    simgrid::s4u::this_actor::execute(1e4);

    // SUB: after the execution of this task, the HDD utilization decreases by 100 (bytes)
    TRACE_host_variable_sub(hostname, "HDD_utilization", 100);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring user variables
  TRACE_host_variable_declare("HDD_capacity");
  TRACE_host_variable_declare("HDD_utilization");

  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), trace_fun);

  e.run();

  // get user declared variables
  unsigned int cursor;
  char* variable;
  xbt_dynar_t host_variables = TRACE_get_host_variables();
  if (host_variables) {
    XBT_INFO("Declared host variables:");
    xbt_dynar_foreach (host_variables, cursor, variable) {
      XBT_INFO("%s", variable);
    }
    xbt_dynar_free(&host_variables);
  }
  xbt_dynar_t link_variables = TRACE_get_link_variables();
  if (link_variables) {
    xbt_assert(xbt_dynar_is_empty(link_variables), "Should not have any declared link variable!");
    xbt_dynar_free(&link_variables);
  }

  return 0;
}
