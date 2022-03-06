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
namespace sg4 = simgrid::s4u;

static void trace_fun()
{
  const auto host = sg4::this_actor::get_host()->get_name();

  // the hostname has an empty HDD with a capacity of 100000 (bytes)
  simgrid::instr::set_host_variable(host, "HDD_capacity", 100000);
  simgrid::instr::set_host_variable(host, "HDD_utilization", 0);

  for (int i = 0; i < 10; i++) {
    // create and execute a task just to make the simulated time advance
    sg4::this_actor::execute(1e4);

    // ADD: after the execution of this task, the HDD utilization increases by 100 (bytes)
    simgrid::instr::add_host_variable(host, "HDD_utilization", 100);
  }

  for (int i = 0; i < 10; i++) {
    // create and execute a task just to make the simulated time advance
    sg4::this_actor::execute(1e4);

    // SUB: after the execution of this task, the HDD utilization decreases by 100 (bytes)
    simgrid::instr::sub_host_variable(host, "HDD_utilization", 100);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring user variables
  simgrid::instr::declare_host_variable("HDD_capacity");
  simgrid::instr::declare_host_variable("HDD_utilization", "1 0 0"); // red color

  sg4::Actor::create("master", e.host_by_name("Tremblay"), trace_fun);

  e.run();

  // get user declared variables
  const auto& host_variables = simgrid::instr::get_host_variables();
  if (not host_variables.empty()) {
    XBT_INFO("Declared host variables:");
    for (const auto& var : host_variables)
      XBT_INFO("%s", var.c_str());
  }
  const auto& link_variables = simgrid::instr::get_link_variables();
  xbt_assert(link_variables.empty(), "Should not have any declared link variable!");

  return 0;
}
