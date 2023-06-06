/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrates how to create a variable load for tasks.
 *
 * We consider the following graph:
 *
 * comm -> exec
 *
 * With a small load each comm task is followed by an exec task.
 * With a heavy load there is a burst of comm before the exec task can even finish once.
 */

#include "simgrid/plugins/task.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_variable_load, "Messages specific for this s4u example");

static void variable_load(simgrid::plugins::TaskPtr t)
{
  XBT_INFO("--- Small load ---");
  for (int i = 0; i < 3; i++) {
    t->enqueue_execs(1);
    simgrid::s4u::this_actor::sleep_for(100);
  }
  simgrid::s4u::this_actor::sleep_until(1000);
  XBT_INFO("--- Heavy load ---");
  for (int i = 0; i < 3; i++) {
    t->enqueue_execs(1);
    simgrid::s4u::this_actor::sleep_for(1);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::plugins::Task::init();

  // Retreive hosts
  auto* tremblay = e.host_by_name("Tremblay");
  auto* jupiter  = e.host_by_name("Jupiter");

  // Create tasks
  auto comm = simgrid::plugins::CommTask::init("comm", 1e7, tremblay, jupiter);
  auto exec = simgrid::plugins::ExecTask::init("exec", 1e9, jupiter);

  // Create the graph by defining dependencies between tasks
  comm->add_successor(exec);

  // Add a function to be called when tasks end for log purpose
  simgrid::plugins::Task::on_end_cb([](const simgrid::plugins::Task* t) {
    XBT_INFO("Task %s finished (%d)", t->get_name().c_str(), t->get_count());
  });

  // Create the actor that will inject load during the simulation
  simgrid::s4u::Actor::create("input", tremblay, variable_load, comm);

  // Start the simulation
  e.run();
  return 0;
}
