/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(test_trace_integration, "Messages specific for this example");

/** test the trace integration cpu model */
static void test_trace(std::vector<std::string> args)
{
  xbt_assert(args.size() == 3,
             "Wrong number of arguments!\nUsage: %s <task computational size in FLOPS> <task priority>",
             args[0].c_str());

  double task_comp_size = std::stod(args[1]);
  double task_prio      = std::stod(args[2]);

  XBT_INFO("Testing the trace integration cpu model: CpuTI");
  XBT_INFO("Task size: %f", task_comp_size);
  XBT_INFO("Task prio: %f", task_prio);

  /* Create and execute a single task. */
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(task_comp_size);
  exec->set_priority(task_prio)->wait();
  XBT_INFO("Test finished");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s test_trace_integration_model.xml deployment.xml\n", argv[0]);

  e.register_function("test_trace", test_trace);
  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);
  e.run();
  return 0;
}
