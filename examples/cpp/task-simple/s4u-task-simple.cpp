/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrate basic use of tasks.
 *
 * We model the following graph:
 *
 * exec1 -> comm -> exec2
 *
 * exec1 and exec2 are execution tasks.
 * comm is a communication task.
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_simple, "Messages specific for this task example");

namespace sg4 = simgrid::s4u;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  // Retrieve hosts
  auto* tremblay = e.host_by_name("Tremblay");
  auto* jupiter  = e.host_by_name("Jupiter");

  // Create tasks
  auto exec1 = sg4::ExecTask::init("exec1", 1e9, tremblay);
  auto exec2 = sg4::ExecTask::init("exec2", 1e9, jupiter);
  auto comm  = sg4::CommTask::init("comm", 1e7, tremblay, jupiter);

  // Create the graph by defining dependencies between tasks
  exec1->add_successor(comm);
  comm->add_successor(exec2);

  // Add a function to be called when tasks end for log purpose
  sg4::Task::on_completion_cb(
      [](const sg4::Task* t) { XBT_INFO("Task %s finished (%d)", t->get_name().c_str(), t->get_count()); });

  // Enqueue two firings for task exec1
  exec1->enqueue_firings(2);

  // Start the simulation
  e.run();
  return 0;
}
