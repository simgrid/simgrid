/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  std::set<simgrid::s4u::Activity*> vetoed;
  e.track_vetoed_activities(&vetoed);

  auto fafard = e.host_by_name("Fafard");

  // Display the details on vetoed activities
  simgrid::s4u::Activity::on_veto.connect([&e](simgrid::s4u::Activity& a) {
    auto& exec = static_cast<simgrid::s4u::Exec&>(a); // all activities are execs in this example

    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_cname(),
             (exec.dependencies_solved() ? "solved" : "NOT solved"),
             (exec.is_assigned() ? "assigned" : "NOT assigned"));
  });

  // Define an amount of work that should take 1 second to execute.
  double computation_amount = fafard->get_speed();

  // Create a small DAG: Two parents and a child
  simgrid::s4u::ExecPtr first_parent  = simgrid::s4u::Exec::init();
  simgrid::s4u::ExecPtr second_parent = simgrid::s4u::Exec::init();
  simgrid::s4u::ExecPtr child         = simgrid::s4u::Exec::init();
  first_parent->add_successor(child);
  second_parent->add_successor(child);

  /*
    std::vector<simgrid::s4u::ExecPtr> pending_execs;
    pending_execs.push_back(first_parent);
    pending_execs.push_back(second_parent);
    pending_execs.push_back(child);
  */

  // Set the parameters (the name is for logging purposes only)
  // + First parent ends after 1 second and the Second parent after 2 seconds.
  first_parent->set_name("parent 1")->set_flops_amount(computation_amount);
  second_parent->set_name("parent 2")->set_flops_amount(2 * computation_amount);
  child->set_name("child")->set_flops_amount(computation_amount);

  // Only the parents are scheduled so far
  first_parent->set_host(fafard);
  second_parent->set_host(fafard);

  // Start all activities that can actually start.
  first_parent->vetoable_start();
  second_parent->vetoable_start();
  child->vetoable_start();

  while (child->get_state() != simgrid::s4u::Activity::State::FINISHED) {
    e.run();
    for (auto* a : vetoed) {
      auto* exec = static_cast<simgrid::s4u::Exec*>(a);

      // In this simple case, we just assign the child task to a resource when its dependencies are solved
      if (exec->dependencies_solved() && not exec->is_assigned()) {
        XBT_INFO("Activity %s's dependencies are resolved. Let's assign it to Fafard.", exec->get_cname());
        exec->set_host(fafard);
      } else {
        XBT_INFO("Activity %s not ready.", exec->get_cname());
      }
    }
    vetoed.clear(); // DON'T FORGET TO CLEAR this set between two calls to run
  }

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
