/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void worker()
{
  // Define an amount of work that should take 1 second to execute.
  double computation_amount = sg4::this_actor::get_host()->get_speed();

  std::vector<sg4::ExecPtr> pending_execs;
  // Create a small DAG
  // + Two parents and a child
  // + First parent ends after 1 second and the Second parent after 2 seconds.
  sg4::ExecPtr first_parent = sg4::this_actor::exec_init(computation_amount);
  pending_execs.push_back(first_parent);
  sg4::ExecPtr second_parent = sg4::this_actor::exec_init(2 * computation_amount);
  pending_execs.push_back(second_parent);
  sg4::ExecPtr child = sg4::Exec::init()->set_flops_amount(computation_amount);
  pending_execs.push_back(child);

  // Name the activities (for logging purposes only)
  first_parent->set_name("parent 1");
  second_parent->set_name("parent 2");
  child->set_name("child");

  // Create the dependencies by declaring 'child' as a successor of first_parent and second_parent
  first_parent->add_successor(child);
  second_parent->add_successor(child);

  // Start the activities.
  first_parent->start();
  second_parent->start();
  // child uses a vetoable start to force it to wait for the completion of its predecessors
  child->vetoable_start();

  // wait for the completion of all activities
  while (not pending_execs.empty()) {
    ssize_t changed_pos = sg4::Exec::wait_any_for(pending_execs, -1);
    XBT_INFO("Exec '%s' is complete", pending_execs[changed_pos]->get_cname());
    pending_execs.erase(pending_execs.begin() + changed_pos);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Actor::create("worker", e.host_by_name("Fafard"), worker);

  sg4::Activity::on_veto_cb([&e](sg4::Activity& a) {
    auto& exec = static_cast<sg4::Exec&>(a);

    // First display the situation
    XBT_INFO("Activity '%s' vetoed. Dependencies: %s; Ressources: %s", exec.get_cname(),
             (exec.dependencies_solved() ? "solved" : "NOT solved"),
             (exec.is_assigned() ? "assigned" : "NOT assigned"));

    // In this simple case, we just assign the child task to a resource when its dependencies are solved
    if (exec.dependencies_solved() && not exec.is_assigned()) {
      XBT_INFO("Activity %s's dependencies are resolved. Let's assign it to Fafard.", exec.get_cname());
      exec.set_host(e.host_by_name("Fafard"));
    }
  });

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
