/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrates how to create a variable load for operations.
 *
 * We consider the following graph:
 *
 * comm -> exec
 *
 * With a small load each comm operation is followed by an exec operation.
 * With a heavy load there is a burst of comm before the exec operation can even finish once.
 */

#include "simgrid/plugins/operation.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(operation_variable_load, "Messages specific for this s4u example");

static void variable_load(simgrid::plugins::OperationPtr op)
{
  XBT_INFO("--- Small load ---");
  for (int i = 0; i < 3; i++) {
    op->enqueue_execs(1);
    simgrid::s4u::this_actor::sleep_for(100);
  }
  simgrid::s4u::this_actor::sleep_until(1000);
  XBT_INFO("--- Heavy load ---");
  for (int i = 0; i < 3; i++) {
    op->enqueue_execs(1);
    simgrid::s4u::this_actor::sleep_for(1);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::plugins::Operation::init();

  // Retreive hosts
  auto tremblay = e.host_by_name("Tremblay");
  auto jupiter  = e.host_by_name("Jupiter");

  // Create operations
  auto comm = simgrid::plugins::CommOp::init("comm", 1e7, tremblay, jupiter);
  auto exec = simgrid::plugins::ExecOp::init("exec", 1e9, jupiter);

  // Create the graph by defining dependencies between operations
  comm->add_successor(exec);

  // Add a function to be called when operations end for log purpose
  simgrid::plugins::Operation::on_end_cb([](const simgrid::plugins::Operation* op) {
    XBT_INFO("Operation %s finished (%d)", op->get_name().c_str(), op->get_count());
  });

  // Create the actor that will inject load during the simulation
  simgrid::s4u::Actor::create("input", tremblay, variable_load, comm);

  // Start the simulation
  e.run();
  return 0;
}
