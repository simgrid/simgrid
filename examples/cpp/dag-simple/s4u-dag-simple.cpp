/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  std::set<sg4::Activity*> vetoed;
  e.track_vetoed_activities(&vetoed);

  auto fafard = e.host_by_name("Fafard");

  // Display the details on vetoed activities
  sg4::Activity::on_veto_cb([](const sg4::Activity& a) {
    const auto& exec = static_cast<const sg4::Exec&>(a); // all activities are execs in this example

    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_cname(),
             (exec.dependencies_solved() ? "solved" : "NOT solved"),
             (exec.is_assigned() ? "assigned" : "NOT assigned"));
  });

  sg4::Activity::on_completion_cb([](sg4::Activity const& activity) {
    const auto* exec = dynamic_cast<sg4::Exec const*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec->get_cname(), exec->get_start_time(),
             exec->get_finish_time());
  });

  // Define an amount of work that should take 1 second to execute.
  double computation_amount = fafard->get_speed();

  // Create a small DAG: Two parents and a child
  sg4::ExecPtr first_parent  = sg4::Exec::init();
  sg4::ExecPtr second_parent = sg4::Exec::init();
  sg4::ExecPtr child         = sg4::Exec::init();
  first_parent->add_successor(child);
  second_parent->add_successor(child);

  // Set the parameters (the name is for logging purposes only)
  // + First parent ends after 1 second and the Second parent after 2 seconds.
  first_parent->set_name("parent 1")->set_flops_amount(computation_amount);
  second_parent->set_name("parent 2")->set_flops_amount(2 * computation_amount);
  child->set_name("child")->set_flops_amount(computation_amount);

  // Only the parents are scheduled so far
  first_parent->set_host(fafard);
  second_parent->set_host(fafard);

  // Start all activities that can actually start.
  first_parent->start();
  second_parent->start();
  child->start();

  while (child->get_state() != sg4::Activity::State::FINISHED) {
    e.run();
    for (auto* a : vetoed) {
      auto* exec = static_cast<sg4::Exec*>(a);

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

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
