/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_waitany, "Messages specific for this s4u example");

static void worker(bool with_timeout)
{
  /* Vector in which we store all pending executions*/
  std::vector<simgrid::s4u::ExecPtr> pending_executions;

  for (int i = 0; i < 3; i++) {
    std::string name = std::string("Exec-") + std::to_string(i);
    double amount    = (6 * (i % 2) + i + 1) * simgrid::s4u::this_actor::get_host()->get_speed();

    simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(amount)->set_name(name);
    pending_executions.push_back(exec);
    exec->start();

    XBT_INFO("Activity %s has started for %.0f seconds", name.c_str(),
             amount / simgrid::s4u::this_actor::get_host()->get_speed());
  }

  /* Now that executions were initiated, wait for their completion, in order of termination.
   *
   * This loop waits for first terminating execution with wait_any() and remove it with erase(), until all execs are
   * terminated.
   */
  while (not pending_executions.empty()) {
    ssize_t pos;
    if (with_timeout)
      pos = simgrid::s4u::Exec::wait_any_for(pending_executions, 4);
    else
      pos = simgrid::s4u::Exec::wait_any(pending_executions);

    if (pos < 0) {
      XBT_INFO("Do not wait any longer for an activity");
      pending_executions.clear();
    } else {
      XBT_INFO("Activity '%s' (at position %zd) is complete", pending_executions[pos]->get_cname(), pos);
      pending_executions.erase(pending_executions.begin() + pos);
    }
    XBT_INFO("%zu activities remain pending", pending_executions.size());
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Tremblay"), worker, false);
  simgrid::s4u::Actor::create("worker_timeout", e.host_by_name("Tremblay"), worker, true);
  e.run();

  return 0;
}
