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

#include <simgrid/plugins/file_system.h>
#include "simgrid/plugins/operation.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(operation_simple, "Messages specific for this operation example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);
  simgrid::plugins::Operation::init();

  // Retrieve hosts
  auto bob  = e.host_by_name("bob");
  auto carl = e.host_by_name("carl");

  // Create operations
  auto exec1 = simgrid::plugins::ExecOp::init("exec1", 1e9, bob);
  auto exec2 = simgrid::plugins::ExecOp::init("exec2", 1e9, carl);
  auto write = simgrid::plugins::IoOp::init("write", 1e7, bob->get_disks().front(), simgrid::s4u::Io::OpType::WRITE);
  auto read  = simgrid::plugins::IoOp::init("read", 1e7, carl->get_disks().front(), simgrid::s4u::Io::OpType::READ);

  // Create the graph by defining dependencies between operations
  exec1->add_successor(write);
  write->add_successor(read);
  read->add_successor(exec2);

  // Add a function to be called when operations end for log purpose
  simgrid::plugins::Operation::on_end_cb([](const simgrid::plugins::Operation* op) {
    XBT_INFO("Operation %s finished (%d)", op->get_name().c_str(), op->get_count());
  });

  // Enqueue two executions for operation exec1
  exec1->enqueue_execs(2);

  // Start the simulation
  e.run();
  return 0;
}
