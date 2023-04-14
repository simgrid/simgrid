/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrate basic use of the operation plugin.
 *
 * We model the following graph:
 *
 * exec1 -> comm -> exec2
 *
 * exec1 and exec2 are execution operations.
 * comm is a communication operation.
 */

#include "simgrid/plugins/operation.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(operation_simple, "Messages specific for this operation example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::plugins::Operation::init();

  // Retrieve hosts
  auto tremblay = e.host_by_name("Tremblay");
  auto jupiter  = e.host_by_name("Jupiter");

  // Create operations
  auto exec1 = simgrid::plugins::ExecOp::create("exec1", 1e9, tremblay);
  auto exec2 = simgrid::plugins::ExecOp::create("exec2", 1e9, jupiter);
  auto comm  = simgrid::plugins::CommOp::create("comm", 1e7, tremblay, jupiter);

  // Create the graph by defining dependencies between operations
  exec1->add_successor(comm);
  comm->add_successor(exec2);

  // Add a function to be called when operations end for log purpose
  simgrid::plugins::Operation::on_end_cb([](simgrid::plugins::Operation* op) {
    XBT_INFO("Operation %s finished (%d)", op->get_name().c_str(), op->get_count());
  });

  // Enqueue two executions for operation exec1
  exec1->enqueue_execs(2);

  // Start the simulation
  e.run();
  return 0;
}
